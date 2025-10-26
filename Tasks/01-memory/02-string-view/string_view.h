#pragma once

#include <cstring>
#include <string>

class StringView {
public:
    StringView(const std::string& str, size_t start_pos = 0, size_t len_substr = std::string::npos)
        : str_ptr(str.c_str()), start_pos(start_pos), len_substr(len_substr) {
        if (start_pos + len_substr > str.length() || len_substr == std::string::npos) {
            this->len_substr = str.length() - start_pos;
        }
    }

    StringView(const char* str) : str_ptr(str), start_pos(0), len_substr(std::string::npos) {
        this->len_substr = std::strlen(str_ptr);
    }

    StringView(const char* str, size_t len) : str_ptr(str), start_pos(0), len_substr(len) {
        if (len_substr > std::strlen(str_ptr)) {
            this->len_substr = std::strlen(str_ptr);
        }
    }

    char operator[](size_t index) const {
        return str_ptr[start_pos + index];
    }

    size_t Size() const {
        return len_substr;
    }

    const char* str_ptr;
    size_t start_pos;
    size_t len_substr;
};
