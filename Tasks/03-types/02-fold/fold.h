#pragma once

#include <vector>

struct Sum {
    int operator()(int a, int b) const {
        return a + b;
    }
};

struct Prod {
    int operator()(int a, int b) const {
        return a * b;
    }
};

struct Concat {
    std::vector<int> operator()(const std::vector<int>& a, const std::vector<int>& b) const {
        std::vector<int> result = a;
        result.insert(result.end(), b.begin(), b.end());
        return result;
    }
};

template <class Iterator, class T, class BinaryOp>
T Fold(Iterator first, Iterator last, T init, BinaryOp func) {
    for (; first != last; ++first) {
        init = func(init, *first);
    }
    return init;
}

class Length {
public:
    Length(int* length) : len_(length) {
        *len_ = 0;
    }

    int operator()(auto, auto) const {
        (*len_)++;
        return 0;
    }

private:
    int* len_;
};
