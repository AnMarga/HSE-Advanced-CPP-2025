# Лекция №6 (14/10/2025). Паттерны проектирования
## Что это?
- типовые методы решения типовых задач
- высокоуровневые идеи, помогающие организовать код
- общеиспользуемые термины, с помощью которых можно описать систему

Паттерн нельзя придумать, его можно обнаружить.

На паттерны не надо молиться. Это просто какой-то инструмент, который в **некоторых** ситуациях позволяет чуть более удобно структурировать код.


## Template Method
Абстрактное определение алгоритма, разбитое на шаги. Классы-потомки переопределяют конкретные шаги, при этом общая структура остаётся неизменной.

Пример:
```cpp
class Animal {
public:
    void DoStuffInTheMorning() {
        GetUp();
        MakeSound();
        AskForFood();
        GoForAWalk();
    }
// ...
};

class Cat : Animal {
    virtual void MakeSound() override {
        Meow();
    }
};

class Dog : Animal {
    virtual void MakeSound() override {
        Bark();
    }
};
```
Логика общего метода не меняется `DoStuffInTheMorning`, но при этом мы можем кастомизировать методы, которые используются внутри алгоритма.


## Singleton
Класс, гарантирующий существование единственного своего представителя, и предоставляющий доступ к нему (самый спорный паттерн).

Самый популярный пример: логгер.

Как реализовывать? `x.h`:
```cpp
#pragma once

int& GetX();
void IncX();
```
`x.cpp`:
```cpp
int x = 0;

int& GetX() {
    return x;
}

void IncX() {
    ++x;
}
```
`main.cpp`:
```cpp
#include "x.h"

#include <iostream>

int main() {
    auto& xRef = GetX();
    std::cout << xRef << std::endl;
    IncX();
    std::cout << xRef << std::endl;
}
```

А что, если мы напишем `int x = 0` в заголовочном файле? Тогда, если мы будем инклюдить хедер в два разных `.cpp`-файла, то у нас будет ошибка при линковке двух этих файлов в итоговый бинарь, потому что мы нарушим `one definition rule`.

Как можно обойти это? Ну, например, использовать ключевое слово `static`:
```cpp
static int x = 0;

int main() {
}
```
Это ключевое слово в данном случае говорит о том, что переменная `x` глобальная только для конкретного этого файла, и она не будет шариться между всеми файлами при линковке.

Мы можем объявить в хедере `static int x = 0`, но скорее всего это не то, чего мы хотели бы добиться, ибо у нас просто будут в каждом `.cpp`-файле независимые глобальные переменные `x`. Это точно не `singleton`.

Никогда не нужно писать в заголовочных файлах переменные кроме действительно каких-то констант, и кроме `static`-переменных классов.

Все шаблоны нужно писать в заголовном файле (полностью), кроме тех случаев, когда шаблон используется только в одном `.cpp`-файле. Тогда его реализацию нужно писать в самом `.cpp`-файле.

Шаблонные функции так же требуется реализовывать в хедере. Обычную реализацию в `.cpp`-файле.

Однако, ключевое слово `inline` позволяет писать в хедере реализацию нешаблонных функций. Это ключевое слово по сути сообщает компилятору: я готов нарушить `one definition rule`. При этом мы должны гарантировать, что функция, которую мы пометили `inline` - она не меняется, она одинаковая во всех местах, где она будет определена, иначе - UB.

Если вы хотите писать сущности в заголовочном файле, то это должны быть либо объявления, либо определения функций `inline`, либо шаблонные определения, либо определения классов.

Для методов классов ключевое слово `inline` добавляется неявно.

### Замечание:
```cpp
// объявление
void Foo();

// определение
void Bar() {
    return 42;
}
```

Маленькие функции, которые редко меняются, но которые часто используются, имеет смысл выносить в заголовочный файл. Вы размениваете время компиляции всех пользователей класса на производительность. Потому что, когда вы компилируете `.cpp`-файл, который видит только объявление - компилятор не может делать предположение о том, как устроена функция, а потому в данном случае он хуже оптимизирует. Чем лучше компилятор видит - тем лучше он оптимизирует. Но всё это палка о двух концах.

`Link-Time Optimization`, или `LTO`, позволяет выжать максимум производительности из вашего кода. Даёт примерно 10% к производительности, и это ОЧЕНЬ много. Есть промежуточный вариант: `Thin-LTO`. Есть вообще продвинутая штука: `PGO`, или `Profile Guide Optimization`.


## ODR (One Definition Rule). Правила
1. Пишите шаблоны целиком в `.h`. Можно сделать `.impl.h`
2. Пишите методы классов либо целиком в `.h` (редко), либо в `.cpp`
3. Пишите объявления обычных функций в `.h`, остальное в `.cpp`
4. Внутри `.cpp` пишите все локальные для модуля сущности в `unnamed namespaces`


## Singleton.Logger
`logger.h`:
```cpp
class Logger {
public:
    void log(const std::string& message) {
        // ...
    }
};

Logger& GetLogger();
```
`logger.cpp`:
```cpp
static Logger logger;

Logger& GetLogger() {
    return logger;
}
```

Однако, есть проблема: вызов конструктора `Logger` произойдёт до захода в `main()`, даже если он ни разу не будет использован.

Для избежания проблем делают более "красивую" версию.
`logger.h`:
```cpp
class Logger {
public:
    static Logger& GetInstance() {
        if (!logger) {
            logger.reset(new Logger);
        }

        return *logger;
    }

private:
    Logger() {
        // ...
    }

    static std::unique_ptr<Logger> logger;
};
```
`logger.cpp`:
```cpp
std::unique_ptr<Logger> Logger::logger;
```

Но это всё ещё не идеал. Лучше всего будет так:
```cpp
Logger& GetLogger() {
    static Logger logger;
    return logger;
}
```
Это почти правильный `singleton`.

Почти правильный синглтон х2:
```cpp
Logger& GetLogger() {
    static Logger* logger = new Logger{};
    return *logger;
}
```
`memory leaks are safe`. Ну правда это не касается циклов, например, потому что там может утечь оооочень много.

Формально это утечка, потому что деструктор никогда не позовётся.

Ещё более красивый и правильный синглтон х3:
```cpp
template<calss T>
T& GetInstance() {
    static T instance;
    return instance;
}
```
Он не знает про `Logger` и это чуть более красиво.


### CRTP (Curiosly Recurring Template Patttern)
`Любытно повторяющийся шаблон шаблона`
```cpp
template<class T>
class Singleton {
public:
    static T& GetInstance() {
        assert(instance);
        return *instance;
    }

    static void InitInstance() {
        assert(!instance);
        instance.reset(new T);
    }

    static void DestroyInstance() {
        assert(instance);
        instance.reset();
    }

    Singleton(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton& operator=(Singleton&&) = delete;
};
```
```cpp
template<calss T>
class Singleton {

    // ...

protected:
    Singleton() = delete;
    static std::unique_ptr<T> instance;
};

template<typename T>
std::unique_ptr<T> Singleton<T>::instance;
```
Теперь мы пишем сам логгер:
```cpp
class Logger {
public:
    Logger() {
        // ...
    }

    void log(const std::string& message) {
        // ...
    }
};

Singleton<Logger>::InitInstance();
Singleton<Logger>::GetInstance().log("I'm a singleton");
```

Но как-то уж слишком много кода, некрасиво выглядит. А потому обычно делают так:
```cpp
class Logger : public Singleton<Logger> {
public:
    Logger() {
        // ...
    }

    void log(const std::string& message) {
        // ...
    }
};
```
Это-то и есть `CRTP`.

Ну а совсем идеальная реализация достигается при помощи `variadic templates`:
```cpp
template<typename... Args>
static void InitInstance(Args&&... args) {
    assert(!instance);
    isntance.reset(new T(std::forward<Args>(args)...));
}

class Logger : public Singleton<Logger>{
public:
    Logger(const std::string& file) {
        // ...
    }

    // ...
};

Logger::InitInstance("log.txt");
Logger::GetInstance().log("I'm a singleton");
```
