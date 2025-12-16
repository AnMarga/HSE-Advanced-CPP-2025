# Лекция №9 (18/11/2025). Condition variable
## Semaphore
Попробуем написать семафор. Семафор - это примитив, он очень похож на мьюеткс. Основное отличие семафора от мьютекса заключается в том, что семафор позволяет иметь несколько потоков в критической секции одновременно.
```cpp
class Semaphore {
public:
    Semaphore(int n) : counter_(n) {
    }
    void Enter();
    void Leave();
private:
    int counter_ = 0;
};
```
- Корзина, в которой лежит n токенов
- Перед входом в критическую секцию поток забирает токен
- После выхода - возвращает токен
- Внутри секции может находиться не более, чем n потоков

Из семафора легко получить мьютекс, просто присвоив `counter_ = 1`.

Семафор есть в стандарте начиная с С++20. `std::counting_semaphore`.

Первая реализация может выглядеть как-то так:
```cpp
class Semaphore {
    std::mutex mutex_;
    int counter_ = 0;
};

void Semaphore::Leave() {
    std::unique_lock<std::mutex> guard(mutex_);
    counter_++;
}
```
`std::unique_lock` удобная обёртка для работы с мьютексами. Автоматически блокирует и разблокирует мьютекс в конструкторе и деструкторе соответственно.

Теперь давайте напишем `Enter()`:
```cpp
class Semaphore {
    std::mutex mutex_;
    int counter_ = 0;
};

void Semaphore::Enter() {
    std::unique_lock<std::mutex> guard(mutex_);
    if (counter > 0) {
        counter_--;
        return;
    } else {
        // ??
    }
}
```
Если у нас есть токены, то всё нормально, просто убавляем счётчик. А если токенов нет? Что делать? Надо ждать. А как? Нас кто-то должен разбудить, когда токены снова появятся.

Есть важная и полезная конструкция под названием `std::atomic`. Она позволяет атомарно, неделимо работать с простыми объектами типа `int` и подобных. И ненужно возиться с массивными и тяжёлыми мьютексами. И она могла бы помочь читать `counter_`, избегая `Data Race`, но нам это ни к чему. Вот впринципе рабочий код, но крайне неэффективный:
```cpp
#include <mutex>
#include <thread>

class Semaphore {
public:
    Semaphore(int n) : counter_(n) {
    }
    void Enter();
    void Leave();
private:
    std::mutex mtx_;
    int counter_ = 0;
};

void Semaphore::Leave() {
    std::unique_lock<std::mutex> guard(mutex_);
    counter_++;
}

void Semaphore::Enter() {
    while (true) {
        {
            std::unique_lock<std::mutex> guard(mutex_);
            if (counter > 0) {
                counter_--;
                return;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}
```
Код крайне неэффективный. Постоянно поток просыпается, лочит мьютекс, проверяет наличие токенов, засыпает. Мы тратим процессорное время просто так и по сути греем воздух.

Давайте всё-таки напишем правильную реализацию, воспользовавшись прекрасной конструкцией `condition_variable`:
```cpp
#include <mutex>
#include <condition_variable>

class Semaphore {
public:
    Semaphore(int n) : counter_(n) {
    }
    void Enter();
    void Leave();
private:
    int counter_ = 0;
    std::mutex mtx_;
    std::condition_variable not_empty_;
};

void Semaphore::Leave() {
    std::lock_guard guard{mtx_};
    counter_++;
    not_empty_.notify_one();
}

void Semaphore::Enter() {
    std::unique_lock<std::mutex> lock{mtx_};
    not_empty_.wait(lock, [this] {
        return counter_ > 0;
    });
    counter_--;
}
```
`std::lock_guard` - обёртка над мьютексом. В конструкторе делает `lock()`, в деструкторе - `unlock()`. `std::unique_lock` - более громоздкая штука, которая уже имеет некоторые методы. Можно залочить мьютекс, разлочить и т.п. Но в остальном логика такая же. Когда хватает `std::lock_guard` - лучше использовать его.

В нашей реализации получается следующая штука: мы лочим мютекс, под ним проверяем `counter_`, и если всё хорошо - возвращаем залоченый мьютекс и дальше выполняем какую-то логику. Но если предикат не исполняется, то мьютекс автоматом разблокируется. А заблокируется только тогда, когда предикат реально будет исполняться.

`notify_one()` оповещает случайный поток о том, что нужно проснуться и проверить предикат. `notify_all()` оповещает все потоки.
