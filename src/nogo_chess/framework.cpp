#pragma warning(disable:4996)
/* This File is part of $PROGRAM_NAME
/  $PROGRAM_NAME is free software: you can redistribute it and/or modify
/  it under the terms of the GNU Lesser General Public License as published by
/  the Free Software Foundation, either version 3 of the License, or
/  (at your option) any later version.
/
/  $PROGRAM_NAME is distributed in the hope that it will be useful,
/  but WITHOUT ANY WARRANTY; without even the implied warranty of
/  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/  GNU Lesser General Public License for more details.
/
/  You should have received a copy of the GNU Lesser General Public License
/  along with $PROGRAM_NAME at /LICENSE.
/  If not, see <http://www.gnu.org/licenses/>. */


#include <stdio.h>
#include <deque>
#include <thread>
#include <windows.h>
#include <namedpipeapi.h>
#include <glm/glm.hpp>
#include <json.h>
#include "framework.h"
#include "function.h"
#include "cppsqlite3.h"
#include "catalog.h"

struct Jsdata {
    bool isfill = false;
    Json::Value response = {};
    Json::Value data = {};
};

Jsdata* PlayerData[2];

int do_Resume(bool isfill, const char* filename, Board* game) {
    int row = 0;
    CppSQLite3DB database = {};
    database.open(filename);
    if (database.tableExists("MAP")) {
        CppSQLite3Table table = database.getTable("SELECT * FROM MAP;");
        bool priority = false;
        for (; row < table.numRows(); row++) {
            table.setRow(row);
            game->step.push_back(glm::ivec2(atoi(table.fieldValue(0)), atoi(table.fieldValue(1))));
            Json::Value action; action["x"] = atoi(table.fieldValue(0)); action["y"] = atoi(table.fieldValue(1));
            PlayerData[priority]->response[row / 2] = action;
            if (isfill) game->board[atoi(table.fieldValue(0))][atoi(table.fieldValue(1))] = priority;
            game->num++;
            priority = !priority;
        }
        database.close();
    }
    return row;
}

void do_Save(const char* filename, Board* game) {
    CppSQLite3DB database = {};
    database.open(filename); 
    if (database.tableExists("MAP")) database.execDML("DROP TABLE MAP");
    database.execDML("CREATE TABLE MAP("
        "X INT NOT NULL," \
        "Y INT NOT NULL);");
    for (int i = 0; i < game->step.size();i++) {
        char sql_exec[256] = {};
        sprintf_s(sql_exec, "INSERT INTO MAP (X, Y) VALUES (%d, %d);", game->step[i].x, game->step[i].y);
        database.execDML(sql_exec);
    }
    database.close();
}

void Player::init_Player(bool n, Board* ptr, Catalog* dbg) {
    id = n;
    map = ptr;
    debug = dbg;
    if (is_AI){
        lpSecurityAttributes.nLength = sizeof(lpSecurityAttributes);
        lpSecurityAttributes.bInheritHandle = TRUE;
        lpSecurityAttributes.lpSecurityDescriptor = NULL;
    }
    if (PlayerData[n] != nullptr) delete PlayerData[n];
    PlayerData[n] = new Jsdata;
}

#define inBorder(x, y) x > -1 && y > -1 && x < MAP_WIDTH && y < MAP_HEIGHT
glm::ivec2 Player::do_Action() {
    if (is_AI) {
        lpProcessInfo = {};
        lpStartupInfo = {};
        ReadBuff = {};
        WriteBuff = {};
        do_Send();
        if (init_Program() != 0) return glm::ivec2(-1);
        do_Threadjoin();
        glm::ivec2 pos = do_Fetch();
        if (!(inBorder(pos.x, pos.y))) return glm::ivec2(-2);
        Json::Value action; action["x"] = pos.x; action["y"] = pos.y;
        PlayerData[id]->response[map->num / 2] = action;
        map->step.push_back(pos);
        if (map->board[pos.x][pos.y] == -1)
            map->board[pos.x][pos.y] = id;
        else return glm::ivec2(-2);
        map->num++;
        return pos;
    } else {
        isInterface_input = true;
        while (isInterface_input && !isTerminal) Sleep(100);
        if (isTerminal) return glm::ivec2(0);
        Json::Value action = {}; action["x"] = pInterface.x; action["y"] = pInterface.y;
        PlayerData[id]->response[map->num / 2] = action;
        map->step.push_back(pInterface);
        if (map->board[pInterface.x][pInterface.y] == -1)
            map->board[pInterface.x][pInterface.y] = id;
        else return glm::ivec2(-2);
        map->num++;
        return pInterface;
    }
}

void Player::do_IOLoop(HANDLE hstdout, HANDLE hstdin) {

    char BitRead = NULL;
    char BitWrite = NULL;
    unsigned long lBytesRead;
    unsigned long lBytesWrite;
    DWORD ExitCode = STILL_ACTIVE;
    bool bSuccess = false;

    while (ExitCode == STILL_ACTIVE) {
        PeekNamedPipe(hstdout, &BitRead, sizeof(BitRead), &lBytesRead, 0, 0);
        if (lBytesRead) {
            bSuccess = ReadFile(hstdout, &BitRead, sizeof(BitRead), &lBytesRead, 0);
            //std::cout << BitRead;
            if (bSuccess && lBytesRead != 0) ReadBuff.push_back(BitRead);
        } else {
            if (!WriteBuff.empty()) {
                BitWrite = WriteBuff.front();
                //std::cout << BitWrite;
                bSuccess = WriteFile(hstdin, &BitWrite, sizeof(BitWrite), &lBytesWrite, 0);
                if (bSuccess && lBytesWrite != 0) WriteBuff.pop_front();
            } else Sleep(15);
        }
        GetExitCodeProcess(lpProcessInfo.hProcess, &ExitCode);
    }
    for(;;){
        PeekNamedPipe(hstdout, &BitRead, sizeof(BitRead), &lBytesRead, 0, 0);
        if (lBytesRead) {
            bSuccess = ReadFile(hstdout, &BitRead, sizeof(BitRead), &lBytesRead, 0);
            //std::cout << BitRead;
            if (bSuccess && lBytesRead != 0) ReadBuff.push_back(BitRead);
        }
        else break;
    }
    for (;;) {
        if (ReadBuff.empty()) break;
        if (ReadBuff.back() != '}') ReadBuff.pop_back();
        else break;
    }
    ReadBuff.push_back('\n');
    ReadBuff.push_back('\0');
    
    CloseHandle(lpProcessInfo.hProcess);
    CloseHandle(lpProcessInfo.hThread);
    CloseHandle(hstdout);
    CloseHandle(hstdin);

}

int Player::init_Program() {

    HANDLE hSTDOUT_RP = NULL;
    HANDLE hSTDOUT_WP = NULL;
    if (!CreatePipe(&hSTDOUT_RP, &hSTDOUT_WP, &lpSecurityAttributes, 0)) return -1;
    if (!SetHandleInformation(hSTDOUT_RP, HANDLE_FLAG_INHERIT, 0)) return -2;

    HANDLE hSTDIN_RP = NULL;
    HANDLE hSTDIN_WP = NULL;
    if (!CreatePipe(&hSTDIN_RP, &hSTDIN_WP, &lpSecurityAttributes, 0)) return -1;
    if (!SetHandleInformation(hSTDIN_WP, HANDLE_FLAG_INHERIT, 0)) return -2;

    lpStartupInfo.cb = sizeof(STARTUPINFO);
    lpStartupInfo.hStdError = hSTDOUT_WP;
    lpStartupInfo.hStdOutput = hSTDOUT_WP;
    lpStartupInfo.hStdInput = hSTDIN_RP;
    lpStartupInfo.dwFlags |= STARTF_USESTDHANDLES;

    if (!CreateProcess(NULL, stringToLPWSTR(GAMEBACKEND_NAME), NULL, NULL, TRUE, 0, NULL, NULL, &lpStartupInfo, &lpProcessInfo)) {
        CloseHandle(lpProcessInfo.hProcess);
        CloseHandle(lpProcessInfo.hThread);
        CloseHandle(hSTDOUT_WP);
        CloseHandle(hSTDOUT_RP);
        CloseHandle(hSTDIN_WP);
        CloseHandle(hSTDIN_RP);
        return -1;
    } else {
        CloseHandle(hSTDOUT_WP);
        CloseHandle(hSTDIN_RP);
    }

    tLoop = std::thread(&Player::do_IOLoop, this, hSTDOUT_RP, hSTDIN_WP);
    return 0;

}

glm::ivec2 Player::do_Fetch() {
    if (!ReadBuff.empty()) {
        glm::vec2 pos = glm::vec2(0);
        Json::Value action = {};
        Json::Reader reader = {};
        char* lBuff = new char[ReadBuff.size()]();
        deque_charToarray_char(ReadBuff, lBuff);
        //std::cout << lBuff;
        reader.parse(lBuff, action);
        delete[] lBuff;
        pos.x = action["response"]["x"].asInt(); pos.y = action["response"]["y"].asInt();
        PlayerData[id]->data = action["data"];

        char str[32];
        sprintf_s(str, "DEBUG::%d:", map->num);
        debug->print(str);
        //debug->print(action["debug"].asString().c_str());
        debug->print("\n");

        return pos;
    }
    else return glm::ivec2(-1);
}

void Player::do_Send() {

    Json::FastWriter writer = {};

    Json::Value root = {};
    Json::Value action = {};

    std::deque<glm::ivec2>::iterator iter = {};

    root["requests"]  = PlayerData[!id]->response;
    root["responses"] = PlayerData[id]->response;
    root["data"] = PlayerData[id]->data;
    std::string str = writer.write(root);
    stringToarray_deque(str, &WriteBuff);
    WriteBuff.pop_back();
    WriteBuff.push_back('\n');
    WriteBuff.push_back('\0');

}

void Player::do_Threadjoin() {
    tLoop.join();
}