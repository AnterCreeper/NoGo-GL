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


#include <fstream>
#include <iostream>
#include <string>
#include "catalog.h"

void Catalog::init(const char* name) {
    fopen_s(&ctg, name, "w");
    if (ctg == NULL) {
        std::cout << (std::string)"Failed to create Log File " + *name + "::Is the filesystem read-only? or the existed file is locked?" << std::endl;
        std::cout << "The Program will be run without recording Debug Message!" << std::endl;
    } else enable = true;
}

void Catalog::print(const char* str) {
    if (enable) fwrite(str, 1, strlen(str), ctg);
    if (console) std::cout << str;
}

void Catalog::close() {
    if (enable) {
        fclose(ctg);
        enable = false;
    }
}