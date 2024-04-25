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


#include <deque>
#include <vector>
#include <iostream>
#include <windows.h>

LPWSTR stringToLPWSTR(std::string orig);
LPCWSTR stringToLPCWSTR(std::string orig);
void vector_charToarray_char(std::vector<char> in, char* out);
void deque_charToarray_char(std::deque<char> in, char* out);
void stringToarray_deque(std::string in, std::deque<char>* out);
std::wstring stringTowchar_t(const std::string c);
std::string wchar_tTostring(const wchar_t* wp);
char* substr(const char* in, int start, int stop);
float clamp(float in, float down, float up);
int clamp(int in, int down, int up);