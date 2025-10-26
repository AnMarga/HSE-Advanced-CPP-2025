#include "string_operations.h"
#include <charconv>
#include <string_view>
#include <string>

/*
Your implementations here
*/

template<typename T>
int GetSize(T& input) {
    return std::to_chars(input).size();
}

template<typename... Args>
std::string StrCat(const Args&... args) {
    size_t total = (GetSize(args) + ... + 0);
    std::string result;
    result.reserve(total);
    (AppendToString(result, args), ...);
    return result;
}
