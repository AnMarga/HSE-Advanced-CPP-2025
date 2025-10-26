#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct State {
    size_t ref_count;
    std::vector<std::string> vec;

    State() : ref_count(1) {
    }

    State(const State& other) : ref_count(1), vec(other.vec) {
    }
};

class COWVector {
public:
    COWVector();
    ~COWVector();

    COWVector(const COWVector& other);
    COWVector& operator=(const COWVector& other);

    COWVector(COWVector&& other) noexcept;
    COWVector& operator=(COWVector&& other) noexcept;

    // Rule of 5?
    // Деструктор (Done)
    // Копирующий конструктор (Done)
    // Оператор присваивания (Done)
    // Перемещающий конструктор (Done)
    // Перемещающий конструктор присваивания (Done)

    size_t Size() const;

    void Resize(size_t size);

    const std::string& Get(size_t at);
    const std::string& Back();

    void PushBack(const std::string& value);

    void Set(size_t at, const std::string& value);

    // Создаёт новый экземпляр вектора
    void Unlink();

private:
    State* state_;
};
