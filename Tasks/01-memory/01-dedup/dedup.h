#pragma once

#include <memory>
#include <vector>
#include <string>

using std::string;

std::vector<std::unique_ptr<string>> Duplicate(const std::vector<std::shared_ptr<string>>& items) {
    std::vector<std::unique_ptr<string>> out(items.size());

    for (size_t i = 0; i < items.size(); ++i) {
        out[i] = std::make_unique<string>(*items[i]);
    }

    return out;
}

std::vector<std::shared_ptr<string>> DeDuplicate(
    const std::vector<std::unique_ptr<string>>& items) {
    std::vector<std::shared_ptr<string>> out(items.size());

    for (size_t i = 0; i < items.size(); ++i) {
        bool flag = true;
        size_t j = 0;

        while (j < i) {
            if (*out[j] == *items[i]) {
                out[i] = out[j];
                flag = false;
            }
            ++j;
        }

        if (flag) {
            out[i] = std::make_shared<string>(*items[j]);
        }
    }

    return out;
}

/*
Отличие умных указателей от сырых:
Не требуется беспокоится о delete и delete[].
Они сами освобождают память, после смерти.

unique_ptr<T> - умный указатель типа T:
Может быть только один объект, который содержит этот адрес.
Короче на область памяти может ссылать только ОДИН объект.
По умолчанию nullptr
Для создания: std::make_unique<T>(val) - C++20
              new int (val) - до C++14
int* pointer = ptr.get()
Начиная с C++20 можно получать адрес и без get()
ptr.reset() - освобождение памяти

shared_ptr<T> - умный указатель типа T:
Позволяет создавать множество объектов
shared_ptr<T>, которые ссылаются на одну область памяти.
Инициализация: std::make_shared<T>(val)

Условие задачи:
  * Функция `DeDuplicate` принимает `std::vector<std::unique_ptr<std::string>> items` и
      должна вернуть `std::vector<std::shared_ptr<std::string>> out`, такой что:
    - `*(out[i]) == *(items[i])`, то есть, на `i`-той позиции стоит объект, равный
       по значению объекту на `i`-ой позиции в исходном векторе.
    - Каждое уникальное значение существует не более чем в одной копии.

  * Функция `Duplicate` проводит обратное преобразование.
*/
