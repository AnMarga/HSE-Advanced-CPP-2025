#include "harakiri.h"

// Your code goes here
std::string AwesomeCallback::operator()() const&& {
    auto res = text_ + "awesomeness";
    delete this;
    return res;
}
