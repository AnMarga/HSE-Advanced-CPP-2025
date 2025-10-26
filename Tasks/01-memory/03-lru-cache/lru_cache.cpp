#include "lru_cache.h"
#include <utility>

LruCache::LruCache(size_t max_size) : max_size_(max_size), lru_list_(), cache_() {
}

void LruCache::Set(const std::string& key, const std::string& value) {
    // проверяем переполненность кэша
    if (cache_.size() + 1 > max_size_) {
        // удаляем самый старый элемент
        std::string old_key = lru_list_.begin()->first;
        lru_list_.pop_front();
        cache_.erase(old_key);
    }
    // end() возвращает итератор после последнего элемента
    // insert() вставляет вызов в конец списка вызов и возвращает итератор
    auto iter = lru_list_.insert(lru_list_.end(), std::make_pair(key, value));
    cache_[key] = iter;  // добавляем пару в кэш
}

// Проверяет есть ли ключ в кэше
// В value записываем значение по ключу для юзера
bool LruCache::Get(const std::string& key, std::string* value) {
    auto iter = cache_.find(key);
    // не нашли по ключу значение
    if (iter == cache_.end()) {
        return false;
    }

    // нашли значение, тогда:
    // ищем в списке обращений нужную пару
    // из неё берём значением и пишем в юзерскую переменную
    // перемещаем пару в конец, как последнее обращение
    auto list_iter = iter->second;
    *value = list_iter->second;
    // this.splice (итератор, куда перемещаем,
    // список, из которого перемещаем (other),
    // итератор в этом списке (other))
    lru_list_.splice(lru_list_.end(), lru_list_, list_iter);
    return true;
}
