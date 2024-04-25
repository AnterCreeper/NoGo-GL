#pragma once
#pragma warning(disable : 4996)
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


#include "catalog.h"
#include "fetch.h"

#include <vector>
#include <thread>
#include <string>

#define IRRKLANG_STATIC
#include <irrKlang.h>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <json.h>

// Definition of Constant
#define CATALOG_NUM 2
#define CATALOG_RENDER 0
#define CATALOG_BACKEND 1

static const char* MAINLOG = "./logs/main.log";
static const char* LOG_NAME[CATALOG_NUM] = { "./logs/render.log",
                                             "./logs/backend.log" };

#define OPTION_NUM 7
static const char* OPTION_NAME[OPTION_NUM] = { "screen_width",
                                               "screen_height",
                                               "fresh_rate",
                                               "is_fullscreen",
                                               "is_sound",
                                               "is_debug",
                                               "is_init"};
static char CONFIG_FILENAME[] = "config.json";

#define SHADER_NUM 4
#define SHADER_TABLE 0
#define SHADER_CHESS 1
#define SHADER_COMPOSITE 2
#define SHADER_FINAL 3
static const char* SHADER_FILENAME[SHADER_NUM] = { "gbuffer_table",
                                                   "gbuffer_chess",
                                                   "composite",
                                                   "final" };

#define AUDIO_NUM 7
#define AUDIO_INIT 0
#define AUDIO_FAILED 1
#define AUDIO_PLACE 2
#define AUDIO_START 3
#define AUDIO_CYC_NAME "./sounds/game.ogg"
static const char* AUDIO_NAME[AUDIO_NUM] = { "./sounds/init.ogg",
                                             "./sounds/failed.ogg",
                                             "./sounds/place.ogg",
                                             "./sounds/ugame01.ogg",
                                             "./sounds/ugame02.ogg",
                                             "./sounds/dgame01.ogg",
                                             "./sounds/dgame02.ogg"};

#define TEXTURE_TABLE_NUM 3
static const char* TEXTURE_TABLE_NAME[TEXTURE_TABLE_NUM] = { "./textures/table.png",
                                                             "./textures/n_table.png",
                                                             "./textures/s_table.png"};
static const GLint TEXTURE_TABLE_FORMAT[TEXTURE_TABLE_NUM] = { GL_RGBA,
                                                               GL_RGBA,
                                                               GL_RGB };
#define TEXTURE_BUTTON_NUM 25
static const char* TEXTURE_BUTTON_NAME[TEXTURE_BUTTON_NUM] = { "./textures/button_l0.png",
                                                               "./textures/button_l1.png",

                                                               "./textures/button_m0.png",
                                                               "./textures/button_m1.png",
                                                               "./textures/button_m2.png",
                                                               "./textures/button_m3.png",

                                                               "./textures/button_r0.png",
                                                               "./textures/button_r1.png",

                                                               "./textures/button_d00.png",
                                                               "./textures/button_d01.png",
                                                               "./textures/button_d02.png",
                                                               "./textures/button_d03.png",
                                                               "./textures/button_d04.png",
                                                               "./textures/button_d05.png",

                                                               "./textures/button_d10.png",
                                                               "./textures/button_d11.png",
                                                               "./textures/button_d12.png",
                                                               "./textures/button_d13.png",
                                                               "./textures/button_d14.png",
                                                               "./textures/button_d15.png",

                                                               "./textures/button_d20.png",
                                                               "./textures/button_d21.png",
                                                               "./textures/button_d22.png",
                                                               "./textures/button_d23.png",

                                                               "./textures/dialog.png" };

#define BUTTON_NUM 7

#define BUTTON_L 0
#define BUTTON_M 1
#define BUTTON_R 2

#define BUTTON_MENU 3
#define BUTTON_D0 4
#define BUTTON_D1 5
#define BUTTON_D2 6

#define BUTTON_MAP_NAME "./textures/define.png"
#define DIALOG_MAP_NAME "./textures/dialog_define.png"

#define FONT_NAME "./fonts/Inkfree.ttf"
#define MODEL_TABLE_NAME "./models/table.obj"
#define MODEL_CHESS_NAME "./models/chess.obj"

#define TALK_NUM 5
static const char* TALK_01[TALK_NUM] = { "Hello  Player,",
                                      "Go  is  a  game  that  doesn't  speak  much",
                                      "Sneak  around  my  chess  every  day!",
                                      "Can  you  help  me?",
                                      "Let's  rock!" };
static const char* TALK_02[TALK_NUM] = { "",
                                      "about  martial  arts. I  lost  again  recently!",
                                      "I  want  to  have  a  final  NoGo  with  him!",
                                      "",
                                      "" };
#define AUDIO_INIT_NUM 5

// Global Configuration Module
class Setting {
private:
    Json::Value root = {};
    int* pointer[OPTION_NUM] = {};
public:
    void save(const char* cfgfile) {
        FileSystem fs = {};
        Json::Value root;
        Json::FastWriter writer;
        fs.init(cfgfile, 1);
        for (int i = 0; i < OPTION_NUM; i++) {
            root[OPTION_NAME[i]] = *pointer[i];
        }
        std::string str = writer.write(root);
        fs.write(str.c_str());
        fs.close();
    }
    bool init(const char* cfgfile) {
        {
            pointer[0] = &option.size.width;
            pointer[1] = &option.size.height;
            pointer[2] = &option.fresh_rate;
            pointer[3] = &option.full_screen;
            pointer[4] = &option.is_sound;
            pointer[5] = &option.is_debug;
            pointer[6] = &option.is_init; }
        FileSystem fs = {};
        Json::Reader conv = {};
        const bool status = fs.init(cfgfile, 0);
        if (!status) return false;
        else {
            fs.fill();
            char* str = new char[fs.length + 1];
            fs.fetch(str);
            conv.parse(str, root);
            delete[] str;
            for (int i = 0; i < OPTION_NUM; i++) {
                *(pointer[i]) = root[OPTION_NAME[i]].asInt();
            }
            fs.close();
            return true;
        }
    }
    struct container {
        struct screen {
            int width = 0;
            int height = 0;
        }size;
        int fresh_rate = 0;
        int full_screen = 0;
        int is_sound = 0;
        int is_debug = 1;
        int is_init = 0;
    }option;
};

// Render Module
struct Vertex {
    // position
    glm::vec3 Position = {};
    // normal
    glm::vec3 Normal = {};
    // texCoords
    glm::vec2 TexCoords = {};
    // tangent
    glm::vec3 Tangent = {};
    // bitangent
    glm::vec3 Bitangent = {};
};
class Shader {
public:
    int init(std::string path, Catalog cfg);
    struct Handle {
        unsigned int main = 0;
        unsigned int vs = 0, fs = 0;
    }handles;
};
class Mesh {
public:
    // Mesh Data
    unsigned int VAO = 0;
    std::vector<Vertex> vertices = {};
    std::vector<unsigned int> indices = {};
    // Constructor
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices) {
        this->vertices = vertices;
        this->indices = indices;
        do_SetupMesh();
    }
    // Render the Mesh
    void do_Draw(Shader shade);
    // Render data 
    unsigned int VBO = 0, EBO = 0;
    // Initializes all the buffer objects/arrays
    void do_SetupMesh();
};
class Texture {
public:
    unsigned int id = 0;
    char* name = NULL;
    bool init(const char* path, GLint mode) {
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        int width, height, nrChannels;
        unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_2D, 0, mode, width, height, 0, mode, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            stbi_image_free(data);
            return true;
        }
        else {
            stbi_image_free(data);
            return false;
        }
    }
};
class Model {
public:
    std::vector<Texture> tex = {};
    Shader shade = {};
    void do_Draw();
    void do_LoadModel(std::string const& path, Catalog* ctlog);
private:
    Catalog* ctg = {};
    std::vector<Mesh> meshes = {};
    void do_ProcessNode(aiNode* node, const aiScene* scene);
    Mesh do_ProcessMesh(aiMesh* mesh, const aiScene* scene);
};

// Audio Module
static irrklang::ISoundEngine* Engine;
class AudioSource {
public:
    irrklang::ISound* Handle = NULL;
    irrklang::ISoundSource* Source = NULL;

    bool isLoop = false;
    void init(irrklang::ISoundEngine** engsrc, const char* fname, bool bLoop) {
        this->isLoop = bLoop;
        this->Source = (*engsrc)->addSoundSourceFromFile(fname);
    }
};
class AudioEngine {
private:
    std::thread* Effect = NULL;
    static void tFadeout(AudioSource& src) {
        const int size = 5E2;
        if (Engine) {
            if (!Engine->isCurrentlyPlaying(src.Source)) return;
            for (int i = 1; i < size + 1; i++) {
                src.Handle->setVolume((double)(size - i) / size);
                Sleep(1);
            }
            Engine->stopAllSoundsOfSoundSource(src.Source);
        }
    }
    static void tFadein(AudioSource& src) {
        const int size = 5E2;
        if (Engine) {
            if (src.Handle) src.Handle->drop();
            src.Handle = Engine->play2D(src.Source, src.isLoop, true, true);
            src.Handle->setVolume(0.0f);
            src.Handle->setIsPaused(false);
            for (int i = 1; i < size + 1; i++) {
                src.Handle->setVolume((double)i / size);
                Sleep(1);
            }
        }
    }
public:
    bool isMute = false;
    inline bool isOpen() {
        return (Engine == NULL);
    }
    inline bool init() {
        Engine = irrklang::createIrrKlangDevice();
        if (!Engine) return false;
        return true;
    }
    inline void play_fade(AudioSource& src) {
        if (Engine) {
            if (!Effect) {
                Effect = new std::thread(&tFadein, src);
            }
            else {
                Effect->join();
                delete Effect;
                Effect = new std::thread(&tFadein, src);
            }
        }
    }
    inline void stop_fade(AudioSource& src) {
        if (Engine) {
            if (!Effect) {
                Effect = new std::thread(&tFadeout, src);
            }
            else {
                Effect->join();
                delete Effect;
                Effect = new std::thread(&tFadeout, src);
            }
        }
    }
    inline void play(AudioSource& src) {
        if (Engine && src.Source) {
            if (src.Handle) src.Handle->drop();
            src.Handle = Engine->play2D(src.Source, src.isLoop, false, true);
        }
    }
    inline void stop(AudioSource& src) {
        if (Engine && src.Source) {
            Engine->stopAllSoundsOfSoundSource(src.Source);
            src.Handle->drop();
            src.Handle = NULL;
        }
    }
    inline void load_source(AudioSource& src, const char* fname) {
        if (Engine) src.Source = Engine->addSoundSourceFromFile(fname);
    }
    inline void join() {
        if (Effect) {
            Effect->join();
            delete Effect;
            Effect = NULL;
        }
    }
    inline void close() {
        if (Engine) Engine->stopAllSounds();
        if (Effect) Effect->join();
        if (Engine) Engine->drop();
    }
    inline void mute() {
        if (Engine) {
            Engine->setSoundVolume(isMute);
            isMute = !isMute;
        }
    }
    inline bool check(AudioSource& src) {
        if (Engine && src.Source) return Engine->isCurrentlyPlaying(src.Source);
        else return false;
    }
};