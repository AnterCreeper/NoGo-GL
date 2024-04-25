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


#include <windows.h>
#include <deque>
#include <vector>
#include <string>
#include "function.h"

LPWSTR stringToLPWSTR(std::string orig) {
    size_t size = orig.length() + 1;
    wchar_t* out = new wchar_t[size]();
    size_t outSize = 0;
    mbstowcs_s(&outSize, out, size, orig.c_str(), size - 1);
    return (LPWSTR)out;
}

LPCWSTR stringToLPCWSTR(std::string orig) {
    size_t origsize = orig.length() + 1;
    size_t convertedChars = 0;
    wchar_t* wcstring = new wchar_t[sizeof(wchar_t) * (orig.length() - 1)]();
    mbstowcs_s(&convertedChars, wcstring, origsize, orig.c_str(), _TRUNCATE);
    return wcstring;
}

void vector_charToarray_char(std::vector<char> in, char* out) {
    std::vector<char>::iterator itor = {};
    int i = 0;
    for (itor = in.begin(); itor != in.end(); itor++) {
        out[i] = *itor;
        i++;
    }
    out[i - 1] = '\0';
}

void deque_charToarray_char(std::deque<char> in, char* out) {
    std::deque<char>::iterator itor = {};
    int i = 0;
    for (itor = in.begin(); itor != in.end(); itor++) {
        out[i] = *itor;
        i++;
    }
    out[i - 1] = '\0';
}

void stringToarray_deque(std::string in, std::deque<char>* out) {
    std::string::iterator iter = {};
    int i = 0;
    for (iter = in.begin(); iter != in.end(); iter++) {
        out->push_back(*iter);
        i++;
    }
    out->pop_back();
    out->push_back('\0');
}

std::wstring stringTowchar_t(const std::string c) {
    size_t m_encode = CP_ACP;
    std::wstring str = {};
    int len = MultiByteToWideChar(m_encode, 0, c.c_str(), c.length(), NULL, 0);
    wchar_t* m_wchar = new wchar_t[len + 1]();
    MultiByteToWideChar(m_encode, 0, c.c_str(), c.length(), m_wchar, len);
    m_wchar[len] = '\0';
    str = m_wchar;
    delete[] m_wchar;
    return str;
}

std::string wchar_tTostring(const wchar_t* wp) {
    size_t m_encode = CP_ACP;
    std::string str = {};
    int len = WideCharToMultiByte(m_encode, 0, wp, wcslen(wp), NULL, 0, NULL, NULL);
    char* m_char = new char[len + 1]();
    WideCharToMultiByte(m_encode, 0, wp, wcslen(wp), m_char, len, NULL, NULL);
    m_char[len] = '\0';
    str = m_char;
    delete[] m_char;
    return str;
}

char* substr(const char* in, int start, int stop) {
    char* out = new char[stop - start + 2];
    for (int i = start; i <= stop; i++) out[i - start] = in[i];
    out[stop - start + 1] = '\0';
    return out;
}

float clamp(float in, float down, float up) {
    return in > down ? (in < up ? in : up) : down;
}
int clamp(int in, int down, int up) {
    return in > down ? (in < up ? in : up) : down;
}