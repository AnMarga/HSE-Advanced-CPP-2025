#pragma once

#include <type_traits>
#include <utility>

// Me think, why waste time write lot code, when few code do trick.
// Если класс пустой и от него можно наследоваться,
// то можно заюзать empty base optimization
template <typename T>
struct EBO {
    static const bool kValue = std::is_empty_v<T> && !std::is_final_v<T>;
};

template <typename T, std::size_t Index, bool UseEBO = EBO<T>::kValue>
struct CompressedPairElem;

// Специализация для kValue = false, для не пустого класса
template <typename T, std::size_t Index>
struct CompressedPairElem<T, Index, false> {
    T value{};

    CompressedPairElem() = default;

    // Нам нужна работа и с lvalue, и с rvalue, а потому
    // используем универсальные ссылки и perfect forwarding
    template <typename U>
    CompressedPairElem(U&& val) : value(std::forward<U>(val)) {
    }

    T& Get() {
        return value;
    }

    const T& Get() const {
        return value;
    }
};

// Специализация для kValue = true, для пустого класса
template <typename T, std::size_t Index>
struct CompressedPairElem<T, Index, true> : private T {
    // Полей нет, наследуемся от пустого T

    CompressedPairElem() = default;

    // Аналогичная ситуация
    template <typename U>
    CompressedPairElem(U&& val) : T(std::forward<U>(val)) {
    }

    T& Get() {
        return *this;
    }
    const T& Get() const {
        return *this;
    }
};

// Итоговый класс CompressedPair
template <typename F, typename S>
class CompressedPair : private CompressedPairElem<F, 0>, private CompressedPairElem<S, 1> {
public:
    using FirstBase = CompressedPairElem<F, 0>;
    using SecondBase = CompressedPairElem<S, 1>;

    CompressedPair() = default;

    // Опять же универсальные ссылочки
    template <typename U, typename V>
    CompressedPair(U&& u, V&& v) : FirstBase(std::forward<U>(u)), SecondBase(std::forward<V>(v)) {
    }

    F& GetFirst() {
        return FirstBase::Get();
    }

    const F& GetFirst() const {
        return FirstBase::Get();
    }

    S& GetSecond() {
        return SecondBase::Get();
    }

    const S& GetSecond() const {
        return SecondBase::Get();
    }
};
