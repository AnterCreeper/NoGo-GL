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


#include <vector>
#include <stdio.h>
#include "function.h"
#include "fetch.h"

bool FileSystem::init(const char* name, unsigned short mode) {
    const char* frase[2] = {"r", "w"};
    fopen_s(&source, name, frase[mode]);
    if (source == NULL) return false;
    else return true;
}

void FileSystem::fill() {
    fseek(source, 0, SEEK_SET);
    while (!feof(source)) {
        content.push_back((char)fgetc(source));
    }
    length = content.size();
}

void FileSystem::fetch(char* str) {
    vector_charToarray_char(content, str);
}

void FileSystem::close() {
    fclose(source);
}
void FileSystem::write(const char* str) {
    if (source) fwrite(str, 1, strlen(str), source);
}