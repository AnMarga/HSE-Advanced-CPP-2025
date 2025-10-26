#pragma once

#include <string>
#include <list>
#include <unordered_map>
#include <utility>

class LruCache {
public:
    LruCache(size_t max_size);

    void Set(const std::string& key, const std::string& value);

    bool Get(const std::string& key, std::string* value);

private:
    // Для удобства
    using Iter = std::list<std::pair<std::string, std::string>>::iterator;

    size_t max_size_;  // размер кэша
    // в конце списка держим последнее обращение
    std::list<std::pair<std::string, std::string>> lru_list_;  // список обращений к объектам
    // мапа, сам кэш, хранит <ключ> - <итератор на объект>
    std::unordered_map<std::string, Iter> cache_;
};

// мапа хранит пару ключ и итератор
// итератор указывает на пару ключ и значение
