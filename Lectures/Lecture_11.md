# Лекция №11 (02/12/2025). Atomic. Lock Free алгоритмы
## Atomic
```cpp
#include <atomic>
#include <mutex>

template<typename T>
class Atomic {
public:
    void Store(T value) {
        std::lock_guard guard{mtx_};
        value_ = value;
    }

    void Load() {
        std::lock_guard guard{mtx_};
        return value_;
    }

private:
    std::mutex mtx_;
    std::atomic<T> value_;
};

class SpinLock {
public:
    void Lock() {
        while (locked_.exchange(true)) {
            // pass
        }
    }

    void Unlock() {
        locked_.store(false);
    }

private:
    std::atomic<bool> locked_ = false;
};

int main() {
    std::atomic<int> x;
    x.store(123);  // записывает значение
    int y = x.load();  // читает значение
}
```
А зачем нам вообще `atomic`, когда примерно то же самое можно реализовать при помощи мьютексов? Вопрос хороший. Кто-то может сказать, что мьютексы тяжелее. Но скорее правда будет в том, что мьютексы требуют исполнения куда большего количества инструкций, чем атомики. Однако, протестировав некоторый код на простеньком бенчмарке, выясняется, что вообще говоря мьютексы работают кратно быстрее атомиков в `Real Time`, и кратно меньше потребляют `CPU Time`.

В принципе утверждение о том, что атомарные переменные - быстрее - в корне неверное. Всё очень сильно заивисит от ситуации, а потому использовать их нужно с умом.

Ключевая особенность атомиков, как мы уже сказали ранее - это количество инструкций. Когда у нас одна реализация требует 1 инструкцию, а другая - n инструкций, то впринципе легко сказать, какой код будет быстрее и эффективнее. И здесь мы переходим к понятиям `Lock Freedom` и `Wait Freedom`.
```cpp
int main() {
    std::atomic<int> x;

    bool is_lock_free = std::atomic<int>::is_always_lock_free;

    x.store(123);
    int y = x.load();
}
```
Короче подробнее о `Lock Free` можно почитать [в этой статье на хабре](https://habr.com/ru/companies/wunderfund/articles/322094/). По сути это многопоточный код без блокировок.

## Lock Free
Как писать `Lock Free` код? Для этого рассмотрим следующую конструкцию:
```cpp
#include <atomic>

template<typename T>
void FetchMax(std::atomic<T>& max, T value) {
    auto prev = max.load();
    if
}

int main() {
    std::atomic<int> max;
    FetechMax(max, 42);
}
```
