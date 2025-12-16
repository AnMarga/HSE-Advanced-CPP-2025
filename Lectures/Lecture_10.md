# Лекция №10 (25/11/2025). Advanced threads (парадигмы мультпоточных вычислений)
## Threads
- Вы создаёте потоки (это дорого!; 5 miccroseconds)
- Ядро хранит контекст - состояние всех регистров и стека. Память хранится общая
- Ядро переключает треды за вас (это тоже дорого!; 3-100 microseconds). Общение с ядром
- Большие сервера ~ 1000-2000 тредов от силы
- Ядро может уйти в себя - там красно-чёрное дерево, иногда при балансировках спайки активностей :)

## Threads. Контроль ядра
Людей, которые хорошо понимают, как устроен планировщик (sheduler) либо единицы, либо вообще нет. Но идейно конструкция довольно понятная. В Linux существуется CFS (completely fairly sheduler).
- Каждый процесс получает долю процессорного времени
- Когда поток создаётся в процессе, планировщик потоков назначает ему сегмент процессорного времени этого процесса; этот временной отрезок называется квантом. Планировщик потоков назначает квант, вычисляет приоритет потока и добавляет его в очередь исполнения.
- Планировщик потоков выбирает первый поток из очереди и запускает. Поток выполняется до тех пор, пока не израсходует свой квант, либо не будет заблокирован (наппример, мьютекс, I/O). Изррасхоодовал свой квант - в очередь. Если он блокируется - удаляется из очереди, пока он не будет разблокирован.

## Параллельные алгоритмы
Существует в stl такая вот странная сущность. Когда её вводили в стандарт, все думали, что делают реально полезную штуку. Однако, на практике это оказалось никому не нужно, ибо вечно возникали вопросы:
- Сколько тредов?
- Вдруг кто-то наплодит сотню тредов?
- А как я могу контролировать количество, хочу две программы на сервере?
- Embedded системы ничего не выигрывают тоже
- Что теперь, параллельность встроена в парадигму плюсов?
- Вы теперь должны думать о том, можно ли что-то распараллелить в одной точке на уровне синтаксиса

Ну один из примеров подобных алгоритмов:
```cpp
std::for_each(std::execution::par, std::begin(a), std::end(a), [&](int i) {
    // ...
});

std::sort(std::execution::par, std::begin(a), std::end(a), [&](int i) {
    // ...
});
```

## Threads. Как решать некоторые проблемы?
- Унифицировать откуда берутся потоки (абсолютно все)
- Уменьшать оверхед ядра (например, Google)
    - Легче учить, сложнее сделать и контролировать
- Менять парадигму вычислений (Facebook, trading, Python, JavaScript...)
    - Асинхронное программирование
        - Сложнее учить, легче сделать
    - Корутины
        - Ещё сложнее учить, легче сделать (в Go ~ легко учить)
    - Акторы, генераторы (накрутки или подмножества корутин)
        - Ещё сложнее учить

Поговорим об унификации потоков. Мы хотим на старте создавать сразу n-е количество потоков, и уже затем распределять на них задачи. Как это сделать?
```cpp
#include <cstddef>
#include <functional>
#include <queue>
#include <vector>
#include <thread>

template<typename T>
class UnboundedBlockingQueue {
public:
    void Push(T value) {
        std::unique_lock guard{mtx_};
        queue_.push(std::move(value));
        not_empty_.notify_one();
    }

    std::optional<T> Pop() {
        std::unique_lock guard{mtx_};
        not_empty_.wait(guard, [this] {
            return !queue_.empty() || cancelled_;
        });

        if (queue_.empty()) {
            return std::nullopt;
        }

        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

    void Cancel() {
        std::unique_lock guard{mtx_};
        cancelled_ = true;
        not_empty_.notify_all();
    }

private:
    bool closed = false;
    std::queue<T> queue_;
    std::mutex mtx_;
    std::conditional_variable not_empty_;
    bool cancelled_ = false;
};

class ThreadPool {
public:
    using Task = std::functional<void()>

    ThreadPool(size_t thread_count) {
        while (thread_count -> 0) {
            workers_.emplace_back([this] {
                RunWorker();
            });
        }
    }

    ~ThreadPool() {
        Stop();
    }

    void Submit(Task task) {
        queue_.Push(std::move(task));
    }

    void Stop() {
        queue_.Cancel();
        for (auto& worker : workers_) {
            worker.join();
        }
        workers_.clear();
    }

private:
    void RunWorker() {
        while (auto task = queue_.Pop()) {
            try {
                (*task)();
            } catch (...) {
                // :)
            }
        }
    }

    std::vector<std::thread> workers_;
    UnboundedBlockingQueue<Task> queue_;
};

int main() {
    ThreadPool tp{10};

    tp.Submit([] {
        printf("Hello from thread pool!\n");
    });

    tp.Submit([] {
        printf("Hello from thread pool!\n");
    });
    
    tp.Submit([] {
        printf("Hello from thread pool!\n");
    });

    tp.Stop();
}
```

`ThreadPool` - базовый строительный блок при написании многопоточного кода.
