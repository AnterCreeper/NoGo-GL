#define PROGRAM_NAME "NoGo_Chess"
#define VERSION      "1.0.20.1017"

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

// TODO: GAME REPLAY / BANNED POSITION 

//HEADERS DEFINITION
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <thread>

#include <windows.h>
#include <ShlObj.h>
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <irrKlang.h>

#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftsizes.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <freetype/fttrigon.h>

//BLOCK DEFINITION
#include "framework.h"
#include "catalog.h"
#include "function.h"
#include "extend.h"

//VARIABLES DEFINITION
Setting config = {};
Catalog mainlog = {};
Catalog catalogs[CATALOG_NUM];

bool isMute = false;
AudioEngine audio;
AudioSource audio_srcs[AUDIO_NUM];
AudioSource audio_cyc_srcs;
AudioSource audio_init_srcs[AUDIO_INIT_NUM];

struct Render {
    GLFWwindow* handle = NULL;
    bool isOpen = false;
    Shader shaders[SHADER_NUM] = {};
    struct Camera {
        glm::vec3 pos   = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 up    = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
    }cam;
    double angle = 3.8f;
    double litlen = 9.2f;
    glm::vec3 litpos = glm::vec3(litlen * cos(angle), 15.0f, litlen * sin(angle));
    Model chesses = {};
    Model table = {};
}window;

bool isBlock = false;
bool isPause = true;
bool isGame = false;
bool isGameTerminal = false;
bool isTerminal = false;
std::thread* tGame = NULL;
std::thread* tWork = NULL;
std::thread* tAudio = NULL;

Board* board_map;
Board* replay;
bool visit_map[9][9] = {};
bool lmap[MAP_WIDTH][MAP_HEIGHT] = {};
bool nmap[MAP_WIDTH][MAP_HEIGHT] = {};

Player* players[2];
double available = 1.0f;

//FUNCTION DEFINITION
//TERMINAL BLOCK
#define FATAL_NUM 7
#define FATAL_OPENGL  0
#define FATAL_CONFIG  1
#define FATAL_SHADER  2
#define FATAL_BACKEND 3
#define FATAL_AUDIO   4
#define FATAL_TEXTURE 5
#define FATAL_FONT    6

const char* ERROR_DESCRIPTION[FATAL_NUM] = { "Failed to create OpenGL windows::Tplace_dy to check and update your OpenGL drivers!",
                                             "Failed to load configuration file::It does not EXIST or is LOCKED by other Programs!",
                                             "Failed to load shader file::$ does not EXIST or is LOCKED by other Programs!",
                                             "Failed to start backend program::Unknown reasons, maybe the program is broken or locked!",
                                             "Failed to start up the audio engine::Please check the audio device!",
                                             "Failed to load texture file::$ does not EXIST or is LOCKED by other Programs!",
                                             "Failed to prepare font texture::Unknown reason."};
void do_Terminal() {
    isTerminal = true;
    isGameTerminal = true;
    isPause = true;
    if (players[0]) players[0]->isTerminal = true;
    if (players[1]) players[1]->isTerminal = true;
    if (tGame != NULL) tGame->join();
    if (tWork != NULL) tWork->join();
    if (tAudio != NULL) tAudio->join();
    audio.close();
    if (window.isOpen) {
        glfwSetWindowShouldClose(window.handle, true);
        for (int i = 0; i < SHADER_NUM; i++) glDeleteProgram(window.shaders[i].handles.main);
        glfwTerminate();
        mainlog.close();
        for (int i = 0; i < CATALOG_BACKEND; i++) catalogs[i].close();
    }
    std::cout << "Have a nice day!" << std::endl;
}
void do_FATAL_Terminal(short code, const char* detail) {
    const char* FATAL_TITLE = "\nFATAL ERROR MESSAGE:\n";
    const char* UNKNOWN_TITLE = "Unknown type of Error.\n";

    char* msg = NULL;
    if (code >= FATAL_NUM) {
        const unsigned int len = strlen(UNKNOWN_TITLE) + 1;
        msg = new char[len]();
        strcpy_s(msg, len, UNKNOWN_TITLE);
    }
    else {
        const unsigned int len = strlen(ERROR_DESCRIPTION[code]) + strlen(detail);
        msg = new char[len]();
        int i = 0;
        while (ERROR_DESCRIPTION[code][i] != '\0') {
            if (ERROR_DESCRIPTION[code][i] == '$') {
                strcat_s(msg, len, substr(ERROR_DESCRIPTION[code], 0, i - 1));
                strcat_s(msg, len, detail);
                strcat_s(msg, len, 
                    substr(ERROR_DESCRIPTION[code], i + 1, strlen(ERROR_DESCRIPTION[code])));
                break;
            }
            i++;
        }
    }

    std::cout << msg;
    MessageBox(NULL, stringToLPCWSTR("Oops! There is a fatal error occured unexpectedly. For details turn to main.log"),
                     stringToLPCWSTR("FATAL ERROR!"), MB_ICONEXCLAMATION);

    mainlog.print(FATAL_TITLE);
    mainlog.print(msg);
    delete[] msg;

    do_Terminal();
    exit(-1);
}

//GAME BLOCK
inline void init_Game(bool ai0, bool ai1) {
    if (board_map) delete board_map;
    board_map = new Board;
    memset(board_map->board, -1, sizeof(board_map->board));
    players[0] = new Player;
    players[1] = new Player;
    players[0]->is_AI = !ai0;
    players[1]->is_AI = !ai1;
}

#define inBorder(x, y) x > -1 && y > -1 && x < MAP_WIDTH && y < MAP_HEIGHT
const int offset_x[] = { -1, 0, 1, 0};
const int offset_y[] = {  0,-1, 0, 1};

inline bool checkEmpty(int h, int w, int flags) {
    if (inBorder(h, w)) {
		if (visit_map[h][w]) return false;
		if (board_map->board[h][w] == -1) return true;
		if (board_map->board[h][w] == flags) {
			visit_map[h][w] = true;
			for (int i = 0; i < 4; i++) 
                if (checkEmpty(h + offset_x[i], w + offset_y[i], flags)) return true;
			return false;
		}
		else return false;
    } else return false;
}
bool checkEnd(int h, int w, bool flags) {
    int empty = 0, enemy = 0;
	for (int i = 0; i < 4; i++) {
		const int x0 = h + offset_x[i], y0 = w + offset_y[i];
		if (inBorder(x0, y0)) {
			if (board_map->board[x0][y0] == -1) ++empty;
			if (board_map->board[x0][y0] == !flags) ++enemy;
		}
		else ++empty, ++enemy;
	}
    if (enemy == 4) return true;
    if (empty == 4) return false;
	bool isEmpty = true;
	for (int i = 0; i < 4; i++) {
		const int x0 = h + offset_x[i], y0 = w + offset_y[i];
        if (inBorder(x0, y0)) {
            const int n = board_map->board[x0][y0];
            if (n != -1) {
                memset(visit_map, 0, sizeof(visit_map));
                isEmpty = isEmpty && checkEmpty(x0, y0, n);
            }
        }
	}
    return !isEmpty;
}

double isBlack = 0.0f;
bool isOver = false;
bool isFlash = false;
bool isInitBlock = false;
bool isChoose = false;
bool isResume = false;
bool isSaved = false;
char* fileResume = NULL;

int dead_player = 2;
int music_num = 3;
glm::vec2 direction = glm::vec2(-90.0f, 0.0f);//yaw, pitch
glm::vec2 last_pos = glm::vec2(config.option.size.width / 2.0, config.option.size.height / 2.0);
void processMouse(GLFWwindow* handle, double xpos, double ypos);
void do_AudioLoop() {
    Sleep(300);
    do {
        if (isGame && !isBlock)
            if (!audio.check(audio_srcs[music_num])) {
                music_num = (music_num - AUDIO_START + 1) % (AUDIO_NUM - AUDIO_START) + AUDIO_START;
                audio.play(audio_srcs[music_num]);
                continue;
            }
        if (isPause && !isGame)
            if (!audio.check(audio_cyc_srcs)) {
                audio.play(audio_cyc_srcs);
                continue;
            }
        Sleep(300);
    } while (!isTerminal);
}
void do_GameLoop() {

    const int num = 5E2;

    players[0]->init_Player(0, board_map, &catalogs[CATALOG_BACKEND]);
    players[1]->init_Player(1, board_map, &catalogs[CATALOG_BACKEND]);

    if(!config.option.is_init) for (int i = 0; i <= num; i++) {
        isBlack = (double)i / (double)num;
        Sleep(2);
    }    
    isPause = false;
    if (audio.check(audio_cyc_srcs)) audio.stop_fade(audio_cyc_srcs);
    else audio.stop_fade(audio_srcs[music_num]);
    char msg[256] = {};
    bool priority = true;
    if (isResume) priority = !(do_Resume(true, fileResume, board_map) % 2);
    audio.join();
    Sleep(800);
    audio.play(audio_srcs[AUDIO_INIT]);
    Sleep(500);
    for (int i = 0; i <= num; i++) {
        isBlack = (double)(num - i) / (double)num;
        Sleep(2);
    }
    isBlock = false;
    config.option.is_init = false;
    isGame = true;
    for (;;) {
        while (isPause && !isGameTerminal) Sleep(100);
        //available = 1.0f - (double)board_map->step.size() / (MAP_WIDTH * MAP_HEIGHT);
        priority = !priority;
        
        glm::ivec2 pos = players[priority]->do_Action();
        if (isGameTerminal) return;
        audio.play(audio_srcs[AUDIO_PLACE]);
        if (pos == glm::ivec2(-1)) do_FATAL_Terminal(FATAL_BACKEND, NULL);
        if (pos == glm::ivec2(-2)) {
            sprintf_s(msg, "Player %d wrong way!", (int)priority);
            catalogs[CATALOG_BACKEND].print(msg);
            catalogs[CATALOG_BACKEND].print("\n");
            break;
        }

        sprintf_s(msg, "%d::Player %d set chess at pos x:%d y:%d.", board_map->num, (int)priority, pos.x, pos.y);
        catalogs[CATALOG_BACKEND].print(msg);
        catalogs[CATALOG_BACKEND].print("\n");
        //std::cout << msg << std::endl;

        if (checkEnd(pos.x, pos.y, priority)) break;
    }

    dead_player = priority;
    audio.play(audio_srcs[AUDIO_FAILED]);
    sprintf_s(msg, "Player %d failed!", (int)priority);
    catalogs[CATALOG_BACKEND].print(msg);
    catalogs[CATALOG_BACKEND].print("\n");
    std::cout << msg << std::endl;

    isBlock = true;
    delete players[0];
    //players[0] = NULL;
    delete players[1];
    //players[1] = NULL;

    isOver = true;
    Sleep(3000);
    audio.stop_fade(audio_srcs[music_num]);
    for (int i = 0; i <= num; i++) {
        isBlack = (double)i / (double)num;
        Sleep(2);
    }
    audio.join();
    isGame = false;
    isOver = false;
    isBlock = false;
    isPause = true;
    Sleep(500);
    isFlash = true;
    processMouse(window.handle, last_pos.x, last_pos.y);
    isFlash = false;
    isOver = false;
    isInitBlock = false;
    for (int i = 0; i <= num; i++) {
        isBlack = (double)(num - i) / (double)num;
        Sleep(2);
    }

    time_t rawtime;
    struct tm* info;
    char buffer[128];
    time(&rawtime);
    info = localtime(&rawtime);
    strftime(buffer, 128, "Saved_%Y%m%d_%H%M%S.db", info);
    do_Save(buffer, board_map);
    config.save(CONFIG_FILENAME);

}
float ReplayTime = 1000;
#define REPLAY_MIN 400.0f
#define REPLAY_MAX 3500.0f
void do_ReplayLoop() {

    init_Game(0, 0);
    const int num = 5E2;
    char msg[256] = {};

    isInitBlock = true;
    replay = new Board;
    players[0]->init_Player(0, replay, &catalogs[CATALOG_BACKEND]);
    players[1]->init_Player(1, replay, &catalogs[CATALOG_BACKEND]);
    int row = do_Resume(false, fileResume, replay);
    audio.play(audio_srcs[AUDIO_INIT]);
    Sleep(800);
    tAudio = new std::thread(do_AudioLoop);
    for (int i = 0; i <= num; i++) {
        isBlack = (double)(num - i) / (double)num;
        Sleep(2);
    }

    bool priority = false;

    for (int i = 0;!replay->step.empty();++i) {

        while (isPause && !isGameTerminal) Sleep(100);
        
        glm::ivec2 pos = replay->step.front();
        replay->step.pop_front();
        if (isGameTerminal) return;
        audio.play(audio_srcs[AUDIO_PLACE]);
        if (pos == glm::ivec2(-1)) do_FATAL_Terminal(FATAL_BACKEND, NULL);
        if (pos == glm::ivec2(-2)) {
            sprintf_s(msg, "Player %d wrong way!", (int)priority);
            catalogs[CATALOG_BACKEND].print(msg);
            catalogs[CATALOG_BACKEND].print("\n");
            break;
        }

        board_map->board[pos.x][pos.y] = priority;
        ++board_map->num;
        sprintf_s(msg, "%d::Player %d set chess at pos x:%d y:%d.", board_map->num, (int)priority, pos.x, pos.y);
        catalogs[CATALOG_BACKEND].print(msg);
        catalogs[CATALOG_BACKEND].print("\n");

        if (checkEnd(pos.x, pos.y, priority)) break;
        Sleep(ReplayTime);

        priority = !priority;

    }

    dead_player = priority;
    audio.play(audio_srcs[AUDIO_FAILED]);
    sprintf_s(msg, "Player %d failed!", (int)priority);
    catalogs[CATALOG_BACKEND].print(msg);
    catalogs[CATALOG_BACKEND].print("\n");
    std::cout << msg << std::endl;

    isOver = true;
    Sleep(3000);
    isBlock = true;
    audio.stop_fade(audio_srcs[music_num]);
    for (int i = 0; i <= num; i++) {
        isBlack = (double)i / (double)num;
        Sleep(2);
    }
    audio.join();
    glfwSetWindowShouldClose(window.handle, true);

}
void processReplay() {

    config.option.is_init = false;
    isGame = true;
    isPause = false;
    isBlack = 1.0f;

    OPENFILENAME open = {};
    wchar_t path[MAX_PATH] = {};

    ZeroMemory(&open, sizeof(OPENFILENAME));
    ZeroMemory(&path, sizeof(path));

    open.lStructSize = sizeof(OPENFILENAME);
    open.lpstrFile = path;
    open.nMaxFile = sizeof(path);
    open.lpstrFilter = stringToLPCWSTR("Game Data (*.db)\0*.db\0\0");
    open.lpstrDefExt = stringToLPCWSTR("db");
    open.nFilterIndex = 1;
    open.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR;
    if (GetOpenFileName(&open)) {
        std::string name = wchar_tTostring(path);
        fileResume = new char[name.length() + 1];
        strcpy(fileResume, name.c_str());
        catalogs[CATALOG_BACKEND].print("Load Game Data:");
        catalogs[CATALOG_BACKEND].print(fileResume);
        catalogs[CATALOG_BACKEND].print("\n");
        tGame = new std::thread(do_ReplayLoop);
    }
    else {
        glfwSetWindowShouldClose(window.handle, true);
    }

}

//RENDER BLOCK
double chess_dx = 2.193f;
double chess_dy = 2.193f;

bool placeStatus = 0;
bool ai[2] = {};
int place_dx = 0, place_dy = 0;
unsigned int texs_fb[6] = {};

class TextureData{
public:
    unsigned char* data = new unsigned char;
    int width = 0, height = 0;
    int channel = 0;
    int origin = 0;
    inline void init(const char* path){
        data = stbi_load(path, &width, &height, &channel, 0);
        //std::cout << stbi_failure_reason() << std::endl;
        if (!data) do_FATAL_Terminal(FATAL_TEXTURE, path);
    }
    inline void init(unsigned char* dataptr, int w, int h, int ch) {
        data = dataptr;
        width = w;
        height = h;
        channel = ch;
    }
}texs_font[512];

TextureData tex_init = TextureData();
TextureData button_map = TextureData();
TextureData render_map = TextureData();
class Button {
private:
    std::vector<TextureData*> tex_stack = {};
public:
    int num = 1;
    int status = 0;
    bool enable = true;
    TextureData* tex = NULL;
    int offsetx = 0, offsety = 0;
    inline void init(int n, ...){
        va_list args;
        va_start(args, n);
        for (int i = 0; i < n; i++)
            tex_stack.push_back(va_arg(args, TextureData*));
        tex = tex_stack[0];
        num = n;
        va_end(args);
    }
    inline void set(int n) {
        tex = tex_stack[n];
        status = n;
    }
}buttons[BUTTON_NUM];

bool isPress = false;
bool isEnterPress = false;
bool isESCPress = false;

inline void processNewGame() {
    isBlock = true;
    isGameTerminal = true;
    if (tGame) {
        if (players[0]) players[0]->isTerminal = true;
        if (players[1]) players[1]->isTerminal = true;
        tGame->join();
        delete tGame;
    }
    isGameTerminal = false;
    init_Game(ai[0], ai[1]);
    tGame = new std::thread(do_GameLoop);
}
int init_num = 0;
void doStart() {
    Sleep(1000);
    ai[0] = false, ai[1] = true;
    processNewGame();
    isInitBlock = true;
    Sleep(1000);
    tAudio = new std::thread(do_AudioLoop);
}
void doInit() {
    if (init_num < 4) if (!audio.check(audio_init_srcs[init_num])) {
        ++init_num;
        audio.play(audio_init_srcs[init_num]);
        if (init_num == 4) {
            isBlock = true;
            tWork = new std::thread(doStart);
            return;
        }
    }
}
void processMouse(GLFWwindow* handle, double xpos, double ypos) {

    const float sensitivity = 0.05;
    int width = config.option.size.width;
    int height = config.option.size.height;
    {
        glm::vec2 offset = glm::vec2(xpos - last_pos.x, last_pos.y - ypos) * sensitivity;
        last_pos = glm::vec2(xpos, ypos);
        if (config.option.is_debug) {
            direction += offset;
            direction.y = clamp(direction.y, -89.9, 89.9);

            glm::vec3 front = glm::vec3(
                cos(glm::radians(direction.x)) * cos(glm::radians(direction.y)),
                sin(glm::radians(direction.y)),
                sin(glm::radians(direction.x)) * cos(glm::radians(direction.y)));
            window.cam.front = glm::normalize(front);
        }}
    if (config.option.is_init) return;
    if (isBlock && !isFlash) return;
    xpos = clamp(xpos, 0,  width - 1);
    ypos = clamp(ypos, 0, height - 1);
    int controlx0 = (int)button_map.data[4 * ((int)ypos * width + (int)xpos) + 0];
    int controly0 = (int)button_map.data[4 * ((int)ypos * width + (int)xpos) + 1];
    int controlz0 = (int)button_map.data[4 * ((int)ypos * width + (int)xpos) + 2];
    int controlx1 = (int)render_map.data[4 * ((height - (int)ypos - 1) * width + (int)xpos) + 0];
    int controly1 = (int)render_map.data[4 * ((height - (int)ypos - 1) * width + (int)xpos) + 1];
    int controlz1 = (int)render_map.data[4 * ((height - (int)ypos - 1) * width + (int)xpos) + 2];

    if (controlx0 == 0 && controly0 == 0 && controlz0 == 255) {
        buttons[BUTTON_L].set(1);
        return;
    }
    else buttons[BUTTON_L].set(0);
    if (controlx0 == 0 && controly0 == 255 && controlz0 == 0) {
        buttons[BUTTON_M].set(audio.isMute * 2 + 1);
        return;
    }
    else buttons[BUTTON_M].set(audio.isMute * 2);
    if (controlx0 == 255 && controly0 == 0 && controlz0 == 0) {
        buttons[BUTTON_R].set(1);
        return;
    }
    else buttons[BUTTON_R].set(0);

    if (isPause) {
        if (controlx0 == 255 && controly0 == 255 && controlz0 == 255) {
            buttons[BUTTON_D2].set(isChoose * 2 + 1);
            return;
        }
        else buttons[BUTTON_D2].set(isChoose * 2 + 0);
		if (!isChoose) {
			if ((controlx0 == 127 && controly0 == 127 && controlz0 == 127)
				|| (controlx0 == 255 && controly0 == 255 && controlz0 == 0)) {
				buttons[BUTTON_D1].set(isGame * 2 + 1);
				return;
			}
			else buttons[BUTTON_D1].set(isGame * 2 + 0);
			if ((controlx0 == 63 && controly0 == 63 && controlz0 == 63)
				|| (controlx0 == 255 && controly0 == 255 && controlz0 == 63)) {
				buttons[BUTTON_D0].set(isGame * 2 + 1);
				return;
			}
			else buttons[BUTTON_D0].set(isGame * 2 + 0);
		}
    }
    else {
        if (players[0] && players[1])
            if (players[0]->isInterface_input || players[1]->isInterface_input) {
                if (controlx1 != 0 && controly1 != 0) {
                    place_dx = controlx1 - 1;
                    place_dy = controly1 - 1;
                    placeStatus = 1;
                }
                else placeStatus = 0;
            }
            else placeStatus = 0;
    }

}
void processKeyboard(GLFWwindow* handle, double delta) {
    double step = delta;
    if (config.option.is_debug) {
        if (glfwGetKey(handle, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(handle, true);
        if (glfwGetKey(handle, GLFW_KEY_W) == GLFW_PRESS)
            window.cam.pos += window.cam.front * glm::vec3(step);
        if (glfwGetKey(handle, GLFW_KEY_S) == GLFW_PRESS)
            window.cam.pos -= window.cam.front * glm::vec3(step);
        if (glfwGetKey(handle, GLFW_KEY_A) == GLFW_PRESS)
            window.cam.pos += glm::normalize(glm::cross(glm::vec3(0, 1, 0), window.cam.front)) * glm::vec3(step);
        if (glfwGetKey(handle, GLFW_KEY_D) == GLFW_PRESS)
            window.cam.pos -= glm::normalize(glm::cross(glm::vec3(0, 1, 0), window.cam.front)) * glm::vec3(step);
        if (glfwGetKey(handle, GLFW_KEY_SPACE) == GLFW_PRESS)
            window.cam.pos.y += step;
        if (glfwGetKey(handle, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
            window.cam.pos.y -= step;

        if (glfwGetKey(handle, GLFW_KEY_UP) == GLFW_PRESS)
            chess_dx -= step * 0.1f;
        if (glfwGetKey(handle, GLFW_KEY_DOWN) == GLFW_PRESS)
            chess_dx += step * 0.1f;
        if (glfwGetKey(handle, GLFW_KEY_LEFT) == GLFW_PRESS)
            chess_dy -= step * 0.1f;
        if (glfwGetKey(handle, GLFW_KEY_RIGHT) == GLFW_PRESS)
            chess_dy += step * 0.1f;

        if (glfwGetKey(handle, GLFW_KEY_MINUS) == GLFW_PRESS)
            window.angle -= step * 10.0f;
        if (glfwGetKey(handle, GLFW_KEY_EQUAL) == GLFW_PRESS)
            window.angle += step * 10.0f;

        if (glfwGetKey(handle, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)
            window.litlen -= step * 10.0f;
        if (glfwGetKey(handle, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS)
            window.litlen += step * 10.0f;
        if (glfwGetKey(handle, GLFW_KEY_1) == GLFW_PRESS)
            window.litpos.y -= step * 10.0f;
        if (glfwGetKey(handle, GLFW_KEY_2) == GLFW_PRESS)
            window.litpos.y += step * 10.0f;
        if (glfwGetKey(handle, GLFW_KEY_0) == GLFW_PRESS)
            do_Save("debug.db", board_map);

    }
    else {

        if (glfwGetKey(handle, GLFW_KEY_DOWN) == GLFW_PRESS)
            ReplayTime -= step * 200.0f;
        if (glfwGetKey(handle, GLFW_KEY_UP) == GLFW_PRESS)
            ReplayTime += step * 200.0f;
        ReplayTime = clamp(ReplayTime, REPLAY_MIN, REPLAY_MAX);

        if (players[0] && players[1]) {
            if (players[0]->isInterface_input || players[1]->isInterface_input) {
                if (glfwGetKey(handle, GLFW_KEY_UP) == GLFW_PRESS)
                    if (placeStatus && !isPress) {
                        place_dy--;
                        place_dy = clamp(place_dy, 0, MAP_HEIGHT - 1);
                        isPress = true;
                    }
                    else placeStatus = 1;
                if (glfwGetKey(handle, GLFW_KEY_DOWN) == GLFW_PRESS)
                    if (placeStatus && !isPress) {
                        place_dy++;
                        place_dy = clamp(place_dy, 0, MAP_HEIGHT - 1);
                        isPress = true;
                    }
                    else placeStatus = 1;
                if (glfwGetKey(handle, GLFW_KEY_LEFT) == GLFW_PRESS)
                    if (placeStatus && !isPress) {
                        place_dx--;
                        place_dx = clamp(place_dx, 0, MAP_WIDTH - 1);
                        isPress = true;
                    }
                    else placeStatus = 1;
                if (glfwGetKey(handle, GLFW_KEY_RIGHT) == GLFW_PRESS)
                    if (placeStatus && !isPress) {
                        place_dx++;
                        place_dx = clamp(place_dx, 0, MAP_WIDTH - 1);
                        isPress = true;
                    }
                    else placeStatus = 1;
                if (glfwGetKey(handle, GLFW_KEY_UP) != GLFW_PRESS &&
                    glfwGetKey(handle, GLFW_KEY_DOWN) != GLFW_PRESS &&
                    glfwGetKey(handle, GLFW_KEY_LEFT) != GLFW_PRESS &&
                    glfwGetKey(handle, GLFW_KEY_RIGHT) != GLFW_PRESS) isPress = false;
            }
            else placeStatus = 0;
            if (glfwGetKey(handle, GLFW_KEY_ENTER) == GLFW_PRESS) if(!isEnterPress){

                int width = config.option.size.width;
                int height = config.option.size.height;

                isEnterPress = true;
                for (int i = 0; i < 2; i++) if (players[i]->isInterface_input) {
                    players[i]->pInterface = glm::ivec2(place_dx, place_dy);
                    players[i]->isInterface_input = false;
                    placeStatus = 0;
                    return;
                }

            }
            if (glfwGetKey(handle, GLFW_KEY_ENTER) != GLFW_PRESS) isEnterPress = false;
        }
        if (isInitBlock) return;
        if (glfwGetKey(handle, GLFW_KEY_ESCAPE) == GLFW_PRESS){
            if (!isESCPress) {
                isESCPress = true;
                if (isGame) {
                    isChoose = false;
                    isPause = !isPause;
                    isFlash = true;
                    processMouse(window.handle, last_pos.x, last_pos.y);
                    isFlash = false;
                } else glfwSetWindowShouldClose(handle, true);
            }
        } else isESCPress = false;
    }
}
void processClick(GLFWwindow* handle, int button, int action, int mods) {

    if (isBlock) return;
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {

        if (config.option.is_init) {
            doInit();
            return;
        }

        int width = config.option.size.width;
        int height = config.option.size.height;

        int controlx0 = (int)button_map.data[4 * ((int)last_pos.y * width + (int)last_pos.x) + 0];
        int controly0 = (int)button_map.data[4 * ((int)last_pos.y * width + (int)last_pos.x) + 1];
        int controlz0 = (int)button_map.data[4 * ((int)last_pos.y * width + (int)last_pos.x) + 2];

        int controlx1 = (int)render_map.data[4 * ((height - (int)last_pos.y - 1) * width + (int)last_pos.x) + 0];
        int controly1 = (int)render_map.data[4 * ((height - (int)last_pos.y - 1) * width + (int)last_pos.x) + 1];
        int controlz1 = (int)render_map.data[4 * ((height - (int)last_pos.y - 1) * width + (int)last_pos.x) + 2];

		if (!isPause && isGame && !isInitBlock) if (controlx0 == 0 && controly0 == 0 && controlz0 == 255)
			if (players[0] && players[1])
				if (players[0]->isInterface_input || players[1]->isInterface_input) {
                    if (board_map->step.size() >= 2) for (int i = 0; i < 2; i++) {
						glm::ivec2 step = board_map->step.back();
						board_map->board[step.x][step.y] = -1;
						board_map->step.pop_back();
						--board_map->num;
					}
					return;
				}
        if (controlx0 == 0 && controly0 == 255 && controlz0 == 0) {
            audio.mute();
            buttons[1].set(audio.isMute * 2 + 1);
            return;
        }
		if (isGame && !isInitBlock) if (controlx0 == 255 && controly0 == 0 && controlz0 == 0) {
			isPause = !isPause;
			isChoose = false;
			isFlash = true;
			processMouse(window.handle, last_pos.x, last_pos.y);
			isFlash = false;
			return;
		}
        
        if (!isPause) if (players[0] && players[1]){
            for (int i = 0; i < 2; i++) if (players[i]->isInterface_input) {
                //std::cout << controlx << controly;
                if (controlx1 != 0 && controly1 != 0) {
                    players[i]->pInterface = glm::ivec2(controlx1 - 1, controly1 - 1);
                    players[i]->isInterface_input = false;
                    placeStatus = 0;
                    return;
                }
            }
        }

        if (isChoose) {
            if (controlx0 == 255 && controly0 == 255 && controlz0 == 0) {
                ai[1] = !ai[1];
                buttons[BUTTON_D1].set(4 + ai[1]);
                return;
            }
            if (controlx0 == 255 && controly0 == 255 && controlz0 == 63) {
                ai[0] = !ai[0];
                buttons[BUTTON_D0].set(4 + ai[0]);
                return;
            }
            if (controlx0 == 255 && controly0 == 255 && controlz0 == 255) {
                isChoose = false;
                processNewGame();
                return;
            }
        }
		else if (isPause) {
			if ((controlx0 == 63 && controly0 == 63 && controlz0 == 63) 
                || (controlx0 == 255 && controly0 == 255 && controlz0 == 63)) {
                isChoose = true;
                isResume = false;
                buttons[BUTTON_D0].set(4 + ai[0]);
                buttons[BUTTON_D1].set(4 + ai[1]);
                isFlash = true;
                processMouse(window.handle, last_pos.x, last_pos.y);
                isFlash = false;
                return;
			}
            if ((controlx0 == 127 && controly0 == 127 && controlz0 == 127)
                || (controlx0 == 255 && controly0 == 255 && controlz0 == 0)) {
                if (!isGame) {
                    OPENFILENAME open = {};
                    wchar_t path[MAX_PATH] = {};

                    ZeroMemory(&open, sizeof(OPENFILENAME));
                    ZeroMemory(&path, sizeof(path));
                    open.lStructSize = sizeof(OPENFILENAME);
                    open.lpstrFile = path;
                    open.nMaxFile = sizeof(path);
                    open.lpstrFilter = stringToLPCWSTR("Game Data (*.db)\0*.db\0\0");
                    open.lpstrDefExt = stringToLPCWSTR("db");
                    open.nFilterIndex = 1;
                    open.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR;
                    if (GetOpenFileName(&open)) {
                        isResume = true;
                        std::string name = wchar_tTostring(path);
                        if (fileResume) delete[] fileResume;
                        fileResume = new char[name.length() + 1];
                        strcpy(fileResume, name.c_str());
                        catalogs[CATALOG_BACKEND].print("Load Game Data:");
                        catalogs[CATALOG_BACKEND].print(fileResume);
                        catalogs[CATALOG_BACKEND].print("\n");
                        isChoose = true;
                        buttons[BUTTON_D0].set(4 + ai[0]);
                        buttons[BUTTON_D1].set(4 + ai[1]);
                        isFlash = true;
                        processMouse(window.handle, last_pos.x, last_pos.y);
                        isFlash = false;
                    }
                    return;
                }
                else {
                    OPENFILENAME open = {};
                    wchar_t path[MAX_PATH] = {};

                    ZeroMemory(&open, sizeof(OPENFILENAME));
                    ZeroMemory(&path, sizeof(path));
                    open.lStructSize = sizeof(OPENFILENAME);
                    open.lpstrFile = path;
                    open.nMaxFile = sizeof(path);
                    open.lpstrFilter = stringToLPCWSTR("Game Data (*.db)\0*.db\0\0");
                    open.nFilterIndex = 1;
                    open.lpstrDefExt = stringToLPCWSTR("db");
                    open.Flags = OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_NOCHANGEDIR;
                    if (GetSaveFileName(&open)) {
                        std::string name = wchar_tTostring(path);
                        do_Save(name.c_str(), board_map);
                        MessageBox(NULL, stringToLPCWSTR("Game Save."),
                            stringToLPCWSTR(PROGRAM_NAME), MB_ICONINFORMATION);
                    }
                    return;
                }
                return;
            }
            if (controlx0 == 255 && controly0 == 255 && controlz0 == 255) {
                if (!isGame) glfwSetWindowShouldClose(window.handle, GLFW_TRUE);
                else {
                    players[0]->isTerminal = true;
                    players[1]->isTerminal = true;
                    isGameTerminal = true;
                    tGame->join();
                    tGame = NULL;
                    delete players[0];
                    players[0] = NULL;
                    delete players[1];
                    players[1] = NULL;
                    isGame = false;
                    isPause = true;
                    audio.stop(audio_srcs[music_num]);
                    isFlash = true;
                    processMouse(window.handle, last_pos.x, last_pos.y);
                    isFlash = false;
                }
                return;
            }
        }

    }
}

glm::mat4 view = glm::mat4(1.0f);
glm::mat4 projection = glm::mat4(1.0f);
inline void init_Uniform() {
    view = glm::lookAt(window.cam.pos, window.cam.pos + window.cam.front, window.cam.up);
    projection = glm::perspective(glm::radians(60.0f),
        (float)config.option.size.width / config.option.size.height, 0.1f, 100.0f);
}
inline void do_UpdateUniform(unsigned int handle) {
    if (isPause && !isGame) {
        glm::mat4 inverseview = glm::inverse(view);
        view = glm::rotate(view, glm::radians((float)glfwGetTime() * 1.2f + 0.3f), glm::vec3(0.2, 0.8, 1.2));
        window.angle = 3.8f + (float)glfwGetTime();
        window.litlen = sin((float)glfwGetTime() * 0.2f) * 12.0f;
        window.litpos = glm::vec3(inverseview * view * glm::vec4(window.litlen * cos(window.angle), 10.0f + 5.0f * cos((float)glfwGetTime()), window.litlen * sin(window.angle), 1.0f));
    }
    else {
        window.angle = 3.8f;
        window.litlen = 9.2f;
        window.litpos = glm::vec3(window.litlen * cos(window.angle), 15.0f, window.litlen * sin(window.angle));
    }
    glUniform3fv(glGetUniformLocation(handle, "camPos"), 1, glm::value_ptr(window.cam.pos));
    window.litpos = glm::vec3(window.litlen * cos(window.angle), window.litpos.y, window.litlen * sin(window.angle));;
    glUniform3fv(glGetUniformLocation(handle, "litPos"), 1, glm::value_ptr(window.litpos));
    glUniformMatrix4fv(glGetUniformLocation(handle, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(handle, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
}
void do_UpdateUniformofChesses(glm::ivec2 &pos) {
    int ready = 0;
    float whiteblack = (board_map->board[pos.x][pos.y] == -1) ? 0.5f : std::max(board_map->board[pos.x][pos.y], 0);
	if (placeStatus) if (pos == glm::ivec2(place_dx, place_dy)) {
		ready = 1;
		if (board_map->num % 2) whiteblack = 1;
		else whiteblack = 0;
	}
	if (!board_map->step.empty()) {
		if (pos == board_map->step.back()) ready = 2;
	}
    glUniform1i(glGetUniformLocation(window.shaders[SHADER_CHESS].handles.main, "ready"), ready);
    glUniform1f(glGetUniformLocation(window.shaders[SHADER_CHESS].handles.main, "whiteblack"), whiteblack);
    glUniform2iv(glGetUniformLocation(window.shaders[SHADER_CHESS].handles.main, "map"), 1, glm::value_ptr(pos));
    glUniformMatrix4fv(glGetUniformLocation(window.shaders[SHADER_CHESS].handles.main, "model"),
        1, GL_FALSE, glm::value_ptr(glm::translate(glm::mat4(1.0f), glm::vec3(chess_dx * (pos.x - MAP_HEIGHT / 2), 0.0f, chess_dy * (pos.y - MAP_WIDTH / 2)))));
}
inline void do_UpdateUniformofTable() {
    glUniform1i(glGetUniformLocation(window.shaders[SHADER_TABLE].handles.main, "diffuse"), 0);
    glUniform1i(glGetUniformLocation(window.shaders[SHADER_TABLE].handles.main, "normal"), 1);
    glUniform1i(glGetUniformLocation(window.shaders[SHADER_TABLE].handles.main, "specular"), 2);
    for (int i = 0; i < window.table.tex.size(); i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, window.table.tex[i].id);
    }
    glUniformMatrix4fv(glGetUniformLocation(window.shaders[SHADER_TABLE].handles.main, "model"),
        1, GL_FALSE, glm::value_ptr(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0, 1.0, 0.0))));
}

const float FB_VERTICES[] = {
    //TexCoords
    0.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 0.0f,

    0.0f, 1.0f,
    1.0f, 0.0f,
    1.0f, 1.0f
};
class FrameBuffer {
public:
    unsigned int fbo = 0;
    GLenum* type = NULL;
    FrameBuffer(unsigned int n, ...) {
        va_list args;
        va_start(args, n);
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        type = new GLenum[n];
        for (int i = 0; i < n; i++) {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, va_arg(args, unsigned int), 0);
            type[i] = GL_COLOR_ATTACHMENT0 + i;
        }
        va_end(args);
    }
};

void inline do_DrawText(const char* str, int startx, int starty, int large) {
    for (int i = 0; i < strlen(str); i++) {
        glTexSubImage2D(GL_TEXTURE_2D, 0, startx,
            starty + texs_font[str[i] + large * 256].origin - texs_font[str[i] + large * 256].height, 
            texs_font[str[i] + large * 256].width, texs_font[str[i] + large * 256].height,
            GL_RGBA, GL_UNSIGNED_BYTE, texs_font[str[i] + large * 256].data);
        startx += texs_font[str[i] + large * 256].width + 5;
    }
}
void do_RenderLoop() {
    unsigned int fbVAO = 0, fbVBO = 0;
    {
        glGenVertexArrays(1, &fbVAO);
        glGenBuffers(1, &fbVBO);
        glBindVertexArray(fbVAO);
        glBindBuffer(GL_ARRAY_BUFFER, fbVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(FB_VERTICES), &FB_VERTICES, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0); }

    for (int i = 0; i < 6; i++) {
        glGenTextures(1, &texs_fb[i]);
        glBindTexture(GL_TEXTURE_2D, texs_fb[i]);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, config.option.size.width, config.option.size.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    unsigned int menu = 0;
    {
        glGenTextures(1, &menu);
        glBindTexture(GL_TEXTURE_2D, menu);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, config.option.size.width,
            config.option.size.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL); }
    const unsigned char* nulldata = new unsigned char[4 * config.option.size.width * config.option.size.height]();

    FrameBuffer fb0(4, texs_fb[0], texs_fb[1], texs_fb[4], texs_fb[5]);
    {
        unsigned int rbo;
        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, config.option.size.width, config.option.size.height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); }  //Render Buffer
    FrameBuffer fb1(2, texs_fb[2], texs_fb[3]);

    int frame = 0;
    double pTime0 = glfwGetTime();
    double pTime1 = glfwGetTime();

    window.isOpen = true;
    glEnable(GL_CULL_FACE);
    while (!glfwWindowShouldClose(window.handle)) {
        {
            frame++;
            double cTime = glfwGetTime();
            if (cTime - pTime0 >= 1.0) {
                if (config.option.is_debug) {
                    char str[16] = {};
                    sprintf(str, "FPS: %4d", frame);
                    std::cout << str << std::endl;
                    std::cout << (int)last_pos.x << "|" << (int)last_pos.y << std::endl;
                    std::cout << std::fixed << std::setprecision(3)
                        << "POS: (" << window.cam.pos.x << ","
                        << window.cam.pos.y << ","
                        << window.cam.pos.z << ")" << std::endl;
                    std::cout << "chess_dx:" << chess_dx << " chess_dy:" << chess_dy << std::endl;
                    std::cout << "yaw:" << direction.x << " pitch:" << direction.y << std::endl;
                    std::cout << window.litlen << window.litpos.y;
                }
                //std::cout << debug_dx << debug_dy;
                frame = 0;
                pTime0 = cTime;
            }
            processKeyboard(window.handle, cTime - pTime1); pTime1 = cTime; }
        //Draw Menus
        {
			glBindTexture(GL_TEXTURE_2D, menu);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0,
				0, config.option.size.width, config.option.size.height,
				GL_RGBA, GL_UNSIGNED_BYTE, nulldata);
            if (!config.option.is_init) {
                for (int i = 0; i < BUTTON_MENU; i++) {
                    glTexSubImage2D(GL_TEXTURE_2D, 0, buttons[i].offsetx,
                        buttons[i].offsety, buttons[i].tex->width, buttons[i].tex->height,
                        GL_RGBA, GL_UNSIGNED_BYTE, buttons[i].tex->data);
                }
                if (isPause) for (int i = BUTTON_MENU; i < BUTTON_NUM; i++) if (buttons[i].enable) {
                    glTexSubImage2D(GL_TEXTURE_2D, 0, buttons[i].offsetx,
                        buttons[i].offsety, buttons[i].tex->width, buttons[i].tex->height,
                        GL_RGBA, GL_UNSIGNED_BYTE, isPause ? buttons[i].tex->data : NULL);
                }
            }
        }
        //Draw Texts
        {
            glBindTexture(GL_TEXTURE_2D, menu);
			if (!config.option.is_init) {
				char str[32] = {};
				const char* boardmsg[3] = { "White   DEAD" ,
											"Black   DEAD" ,
											"Just   have  fun!" };
				if (isGame) {
					if (isOver) sprintf_s(str, boardmsg[dead_player]);
					else sprintf_s(str, "ROUND   %02d", board_map->num);
				}
				else sprintf_s(str, boardmsg[2]);
				int startx = 0, starty = 0;
				for (int i = 0; i < strlen(str); i++) {
					startx += texs_font[str[i] + 256].width + 5;
					starty = std::max(texs_font[str[i] + 256].origin, starty);
				}
				startx = (config.option.size.width - startx) / 2;
				starty = config.option.size.height - starty;
				do_DrawText(str, startx, starty, 1);
			}
            else {
                do_DrawText("MorroWind:", 64, 150, 0);
                do_DrawText(TALK_01[init_num], 64, 120, 0);
                do_DrawText(TALK_02[init_num], 64, 90, 0);
            }
        }
        //Draw Tables and chesses.
        {
            init_Uniform();
            //Drawing Objects
            {
                glBindFramebuffer(GL_FRAMEBUFFER, fb0.fbo);
                glEnable(GL_MULTISAMPLE);
                glEnable(GL_DEPTH_TEST);

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDepthRange(0.1f, 100.0f);
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                //Draw Table
                {
                    glUseProgram(window.shaders[SHADER_TABLE].handles.main);
                    do_UpdateUniform(window.shaders[SHADER_TABLE].handles.main);
                    do_UpdateUniformofTable();
                    window.table.do_Draw(); }
                //Draw Chesses
                {
                    for (int i = 0; i < MAP_HEIGHT; i++) for (int j = 0; j < MAP_WIDTH; j++) 
                    if (board_map->board[i][j] != -1 || isGame) {
                        glUseProgram(window.shaders[SHADER_CHESS].handles.main);
                        do_UpdateUniform(window.shaders[SHADER_CHESS].handles.main);
                        do_UpdateUniformofChesses(glm::ivec2(i, j));
                        window.chesses.do_Draw();
                    }}
                glDrawBuffers(4, fb0.type); }
            //Get Chess Map
            {
                glBindTexture(GL_TEXTURE_2D, texs_fb[1]);
                glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, render_map.data);}
            //Composite PASS 1
            {
                glBindFramebuffer(GL_FRAMEBUFFER, fb1.fbo);

                glDisable(GL_DEPTH_TEST);
                glDisable(GL_BLEND);
                glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);

                glUseProgram(window.shaders[SHADER_COMPOSITE].handles.main);
                glUniform1i(glGetUniformLocation(window.shaders[SHADER_COMPOSITE].handles.main, "screenTexture"), 0);
                glUniform1i(glGetUniformLocation(window.shaders[SHADER_COMPOSITE].handles.main, "depthTexture"), 1);
                glUniform1i(glGetUniformLocation(window.shaders[SHADER_COMPOSITE].handles.main, "normalTexture"), 2);
                glUniform1f(glGetUniformLocation(window.shaders[SHADER_COMPOSITE].handles.main, "viewHeight"), config.option.size.height);
                glUniform1f(glGetUniformLocation(window.shaders[SHADER_COMPOSITE].handles.main, "viewWidth"), config.option.size.width);

                glUniformMatrix4fv(glGetUniformLocation(window.shaders[SHADER_COMPOSITE].handles.main, "projectioninverse"), 1, GL_FALSE, glm::value_ptr(glm::inverse(projection)));

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texs_fb[0]);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, texs_fb[4]);
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, texs_fb[5]);
                glBindVertexArray(fbVAO);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                glDrawBuffers(2, fb1.type); }
            //Final Composite
            {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glDisable(GL_DEPTH_TEST);
                glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                glUseProgram(window.shaders[SHADER_FINAL].handles.main);
                glUniform1i(glGetUniformLocation(window.shaders[SHADER_FINAL].handles.main, "screenTexture0"), 0);
                glUniform1i(glGetUniformLocation(window.shaders[SHADER_FINAL].handles.main, "screenTexture1"), 1);
                glUniform1i(glGetUniformLocation(window.shaders[SHADER_FINAL].handles.main, "menuTexture0"), 2);
                glUniform1f(glGetUniformLocation(window.shaders[SHADER_FINAL].handles.main, "viewHeight"), config.option.size.height);
                glUniform1f(glGetUniformLocation(window.shaders[SHADER_FINAL].handles.main, "viewWidth"), config.option.size.width);
                glUniform1f(glGetUniformLocation(window.shaders[SHADER_FINAL].handles.main, "isBlack"), isBlack);
                glUniform1f(glGetUniformLocation(window.shaders[SHADER_FINAL].handles.main, "isInit"), config.option.is_init);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texs_fb[2]);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, texs_fb[3]);
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, menu);

                glBindVertexArray(fbVAO);
                glDrawArrays(GL_TRIANGLES, 0, 6); }
            glfwSwapBuffers(window.handle);
            glfwPollEvents(); }
    }
}

FT_Face* init_FreeType(const char* fontpath) {
    FT_Face* ftFace = new FT_Face;
    FT_Library libraplace_dy;
    if (FT_Init_FreeType(&libraplace_dy) != FT_Err_Ok) do_FATAL_Terminal(FATAL_FONT, NULL);
    if (FT_New_Face(libraplace_dy, fontpath, 0, ftFace) != FT_Err_Ok) do_FATAL_Terminal(FATAL_FONT, NULL);
    FT_Select_Charmap(*ftFace, FT_ENCODING_UNICODE);
    return ftFace;
}
void init_GLFW() {
    glfwInit();
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

    glfwWindowHint(GLFW_CENTER_CURSOR, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_REFRESH_RATE, config.option.fresh_rate);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, GLFW_DONT_CARE);
}
void init_Windows() {
    window.handle = glfwCreateWindow(config.option.size.width, config.option.size.height,
        PROGRAM_NAME, config.option.full_screen ? glfwGetPrimaryMonitor() : NULL, NULL);

    if (window.handle == NULL) do_FATAL_Terminal(FATAL_OPENGL, NULL);

    glfwMakeContextCurrent(window.handle);
    glfwSetCursorPosCallback(window.handle, processMouse);
    glfwSetMouseButtonCallback(window.handle, processClick);
    glfwSetInputMode(window.handle, GLFW_CURSOR, config.option.is_debug ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) do_FATAL_Terminal(FATAL_OPENGL, NULL);
}
void init_Camera() {
    window.cam.pos = glm::vec3(0.0f, 18.90f, 4.60f);
    direction = glm::vec2(-90.0f, -78.0f);
    glm::vec3 front = glm::vec3(
        cos(glm::radians(direction.x)) * cos(glm::radians(direction.y)),
        sin(glm::radians(direction.y)),
        sin(glm::radians(direction.x)) * cos(glm::radians(direction.y)));
    window.cam.front = glm::normalize(front);
}

int main(int argc, char* argv[]) {

    bool isReplay = argc - 1;
    std::cout << PROGRAM_NAME << " v" << VERSION << std::endl;
    //Load and Prepare Resources.
    {
        //Initializing Catalogs.
        {
            mainlog.init(MAINLOG);
            for (int i = 0; i < CATALOG_NUM; i++) catalogs[i].init(LOG_NAME[i]);
            mainlog.print(PROGRAM_NAME " v" VERSION "\n"); }
        //Initializing Configuration
        {
            mainlog.print("Loading Profiles...\n");
            if (!config.init(CONFIG_FILENAME)) do_FATAL_Terminal(FATAL_CONFIG, NULL);
            mainlog.console = config.option.is_debug;
            if (config.option.is_debug)
                std::cout << "**START DEBUG MODE**" << std::endl; }
        //Initializing OpenGL and Windows.
        {
            mainlog.print("Initalizing OpenGL and Creating Windows...\n");
            init_GLFW();
            init_Windows(); }
        //Initializing Audio Engine
        {
            if (config.option.is_sound) {
                mainlog.print("Starting Sound Engine...\n");
                if (!audio.init()) do_FATAL_Terminal(FATAL_AUDIO, NULL);
                mainlog.print("Checking Sound Resources...\n");
                for (int i = 0; i < AUDIO_NUM; i++) audio.load_source(audio_srcs[i], AUDIO_NAME[i]);
                audio.load_source(audio_cyc_srcs, AUDIO_CYC_NAME);
                if (config.option.is_init) for (int i = 0; i < AUDIO_INIT_NUM; i++) {
                    char str[12];
                    sprintf(str, "./sounds/start%d.ogg", i);
                    audio.load_source(audio_init_srcs[i], str);
                };
            }
            else mainlog.print("Sound is disabled according to the configuration...\n");
        }
        //Initializing Shader Program.
        {
            const char* suffix[2] = { ".vsh", ".fsh" };
            mainlog.print("Compiling Shaders...\n");
            for (int i = 0; i < SHADER_NUM; i++) {
                int status = window.shaders[i].init(SHADER_FILENAME[i], catalogs[CATALOG_RENDER]);
                if (status) do_FATAL_Terminal(FATAL_SHADER, (SHADER_FILENAME[i] + (std::string)suffix[status - 1]).c_str());
            }}
        //Initializing Rendering Resources.
        {
            mainlog.print("Loading Textures...\n");
            //Load Table Texture%
            for (int i = 0; i < TEXTURE_TABLE_NUM; i++) {
                window.table.tex.push_back({});
                if (!window.table.tex[i].init(TEXTURE_TABLE_NAME[i], TEXTURE_TABLE_FORMAT[i]))
                    do_FATAL_Terminal(FATAL_TEXTURE, TEXTURE_TABLE_NAME[i]);
            }
            //Load Button Texture
            {
                TextureData button[TEXTURE_BUTTON_NUM];
                for (int i = 0; i < TEXTURE_BUTTON_NUM; i++) button[i].init(TEXTURE_BUTTON_NAME[i]);
                //Right Corner L
                buttons[0].init(2, &button[0], &button[1]);
                buttons[0].offsetx = config.option.size.width - buttons[0].tex->width * 3;
                buttons[0].offsety = config.option.size.height - buttons[0].tex->height;
                //Right Corner M
                buttons[1].init(4, &button[2], &button[3], &button[4], &button[5]);
                buttons[1].offsetx = config.option.size.width - buttons[1].tex->width * 2;
                buttons[1].offsety = config.option.size.height - buttons[1].tex->height;
                //Right Corner R
                buttons[2].init(2, &button[6], &button[7]);
                buttons[2].offsetx = config.option.size.width - buttons[2].tex->width;
                buttons[2].offsety = config.option.size.height - buttons[2].tex->height;

                //Menu
                buttons[3].init(1, &button[24]);
                buttons[3].offsetx = (config.option.size.width - buttons[3].tex->width) / 2;
                buttons[3].offsety = 257;
                //Menu U
                buttons[4].init(6, &button[8], &button[9], &button[10], &button[11], &button[12], &button[13]);
                buttons[4].offsetx = 442;
                buttons[4].offsety = 524;
                //Menu M
                buttons[5].init(6, &button[14], &button[15], &button[16], &button[17], &button[18], &button[19]);
                buttons[5].offsetx = 442;
                buttons[5].offsety = 422;
                //Menu D
                buttons[6].init(4, &button[20], &button[21], &button[22], &button[23]);
                buttons[6].offsetx = 442;
                buttons[6].offsety = 317; }
            //Load Control Texture
            {
                button_map.init(BUTTON_MAP_NAME);
                if (!button_map.data) do_FATAL_Terminal(FATAL_TEXTURE, BUTTON_MAP_NAME);

                render_map.init(new unsigned char[config.option.size.width * config.option.size.height * 4],
                    config.option.size.width, config.option.size.height, 4);}
            //Initializing Rendering Fonts.
            {
                FT_Face* ftFace = init_FreeType(FONT_NAME);
                FT_Set_Pixel_Sizes(*ftFace, 64, 0);
                for (int j = 0; j < 2; j++) {
                    FT_Set_Char_Size(*ftFace, (24 + j * 16) * 92, 0, 72, 0);
                    for (int n = 0; n < 256; n++) {
                        FT_Load_Char(*ftFace, n, FT_LOAD_FORCE_AUTOHINT |
                            (TRUE ? FT_LOAD_TARGET_NORMAL : FT_LOAD_MONOCHROME | FT_LOAD_TARGET_MONO));
                        FT_Glyph glyph = {};
                        FT_Get_Glyph((*ftFace)->glyph, &glyph);
                        FT_Render_Glyph((*ftFace)->glyph, FT_RENDER_MODE_LCD);
                        FT_Glyph_To_Bitmap(&glyph, ft_render_mode_normal, 0, 1);
                        FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)glyph;
                        FT_Bitmap& bitmap = bitmap_glyph->bitmap;

                        int width = bitmap.width;
                        int height = bitmap.rows;
                        (*ftFace)->size->metrics.y_ppem;
                        (*ftFace)->glyph->metrics.horiAdvance;

                        texs_font[n + j * 256].init(new unsigned char[width * height * 4], width, height, 4);
                        texs_font[n + j * 256].origin = (*ftFace)->glyph->bitmap_top;
                        for (int s = 0; s < height; s++) {
                            for (int t = 0; t < width; t++) {
                                unsigned char bit = bitmap.buffer[s * bitmap.width + t];
                                texs_font[n + j * 256].data[4 * ((height - s - 1) * width + t)] = 0xFF;
                                texs_font[n + j * 256].data[4 * ((height - s - 1) * width + t) + 1] = 0xFF;
                                texs_font[n + j * 256].data[4 * ((height - s - 1) * width + t) + 2] = 0xFF;
                                texs_font[n + j * 256].data[4 * ((height - s - 1) * width + t) + 3] = bit;
                            }
                        }
                    }
                }}
            //Initializing Rendering Models.
            {
                mainlog.print("Loading Models...\n");
                window.table.do_LoadModel(MODEL_TABLE_NAME, &catalogs[CATALOG_RENDER]);
                window.table.shade = window.shaders[SHADER_TABLE];

                window.chesses.do_LoadModel(MODEL_CHESS_NAME, &catalogs[CATALOG_RENDER]);
                window.chesses.shade = window.shaders[SHADER_CHESS]; }}
        //Initializing Camera.
        init_Camera(); }
    //Starting Framework Threads.
	mainlog.print("Initializing Game Framework...\n");
	board_map = new Board;
	memset(board_map->board, -1, sizeof(board_map->board));
	mainlog.print("Starting Game...\n");
    srand(time(NULL));
    music_num = rand() % (AUDIO_NUM - AUDIO_START) + AUDIO_START;
    if (!isReplay) {
        if (!config.option.is_init) tAudio = new std::thread(do_AudioLoop);
        else audio.play(audio_init_srcs[0]);
    }
    //Starting Render in main Thread.
    mainlog.print(("Hello " + (std::string)PROGRAM_NAME).c_str());
    mainlog.print("!\n");
    if (config.option.is_init) isBlack = 1.0f;
    if (isReplay) {
        mainlog.print("Start Replay Mode!\n");
        std::cout << "Start Replay Mode!" << std::endl;
        processReplay();
    }
    do_RenderLoop();
    do_Terminal();
    return 0;
}