#pragma once
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
#include "catalog.h"

#define GAMEBACKEND_NAME "NoGo_Backend.exe"

#define MAP_WIDTH  9
#define MAP_HEIGHT 9

struct Board {
    int num = 0;
    int board[MAP_WIDTH][MAP_HEIGHT] = {};
    std::deque<glm::ivec2> step = {};
};
class Player {
private:
    STARTUPINFO lpStartupInfo = {};
    PROCESS_INFORMATION lpProcessInfo = {};
    SECURITY_ATTRIBUTES lpSecurityAttributes = {};
    std::deque<char> ReadBuff = {};
    std::deque<char> WriteBuff = {};

    std::thread tLoop;
    void do_IOLoop(HANDLE hstdin, HANDLE hstdout);
    void do_Threadjoin();

    glm::ivec2 do_Fetch();
    void       do_Send();

    int init_Program();
    Board* map = NULL;
    bool id = false;
public:
    bool isInterface_input = false;
    glm::ivec2 pInterface = glm::ivec2(-1);
    bool isTerminal = false;
    Catalog* debug = NULL;
    bool is_AI = false;
    void init_Player(bool n, Board* ptr, Catalog* dbg);
    glm::ivec2 do_Action();
};

int do_Resume(bool isfill, const char* filename, Board* game);
void do_Save(const char* filename, Board* game);