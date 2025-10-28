# Лекция №7 (21/10/2025). Метапрограммирование
## Что это?
Метапрограмма порождает другую программу, как результат своей работы.

Например, шаблоны в плюсах. Или программа на питоне, которая генерит код на плюсах.

Из базового:
- макросы
- хитрости с шаблонами
- constexpr-выражения (since C++11)
- метаклассы (C++26/...)


## Макросы
- простая текстовая замена
```cpp
#define MAX_SIZE=100

int array[MAX_SIZE];
```
- функциональные макросы
```cpp
#define sqr(x) ((x) * (x))

sqr(2 + 3) -> ((2 + 3) * (2 + 3))
sqr(heavy_func()) -> ((heavy_func()) * (heavy_func()))
```
- условия на этапе компиляции
```cpp
#if defined(__unix__)
#include <unistd.h>
#elif defined(_WIN32)
#include <Windows.h>
#else
#error "unsupported platform"
#endif
```
- проверки
```cpp
#include <cassert>

assert(x > 5);
```

Примеры реализации собственных макросов:
```cpp
#define ENSURE_1(cond) \
    if (!(cond)) { \
        throw std::runtime_error{"Assertion " #cond " failed:"}; \
    }

#define ENSURE_2(cond, msg) \
    if (!(cond)) { \
        throw std::runtime_error{"Assertion " #cond " failed: " msg}; \
    }

#define SELECT_THIRD(a, b, c, ...) c

#define ENSURE(...) SELECT_THIRD(__VA_ARGS__, ENSURE_2, ENSURE_1)(__VA_ARGS__)
```
Плюсы конкатенируют несколько строковых литералов, идущих через пробел, в одну строчку.

С такой реализацией мы можем использовать наши макросы просто как `ENSURE`:
```cpp
int main() {
    ENSURE(1 == 2);
    ENSURE(1 == 2, "uh oh");
}
```

- тестирование (например, библиотека `catch2`)
- ...
```cpp
enum Color {
    Red,
    Green,
    Blue,
};

// Это всё грустно, ибо:
// - а вдруг цветов много
// - а вдруг цвета изменятся
// и т.п.
std::string ToString(Color color) {
    switch (color) {
        case Red:
            return "Red";
        case Green:
            return "Green";
        case Blue:
            return "Blue";
        default:
            throw std::runtime_error{"Unknown color value"};
    }
}

// Есть вариант прикольный, и куда более удобный:
// написать скрипт на питоне, который будет парсить enum
// и создавать как раз таки готовую функцию ToString

// Кстати говоря, в С++26 такая функция появится в стандарте
// Она будет превращать enum в строку
```
```cpp
// Но в то же время эту задачу можно решить чисто на макросах
#define FOR_EACH_COLOR(XX) \
    XX(Red) \
    XX(Green) \
    XX(Blue)

enum Color {
#define POPULATE_ENUM(Value) Value,
    FOR_EACH_COLOR(POPULATE_ENUM)
#undef POPULATE_ENUM
};

std::string ToString(Color color) {
    switch (color) {
#define POPULATE_SWITCH(value) case value: return #value;
        FOR_EACH_COLOR(POPULATE_SWITCH)
#undef POPULATE_SWITCH
    default:
        throw std::runtime_error{"Unknown color value"};
    }
}

// Теперь новые цвета достаточно добавить в макрос FOR_EACH_COLOR
// и всё будет автоматически работать

// Но конечно удобство крайне сомнительное :)
```
А называется всё это `X-макросы`.

На самом деле макросы это довольно мощный язык.


## Шаблоны
- Рекурсивное инстанцирование:
```cpp
template<int N>
struct Factorial {
    static constexpr int value = Factorial<N - 1>::value * N;
};

template<>
struct Factorial<0> {
    static constexpr int value = 1;
};

std::cout << Factorial<8>::value << std::endl;  // mov esi, 40320
```

- `if/else`:
```cpp
template<bool cond, class T, class F>
struct If;

template<typename T, typename F>
struct If<false, T, F> {
    static constexpr int value = F::value;
}

template<typename T, typename F>
struct If<true, T, F> {
    static constexpr int value = T::value;
}
```
- `SFINAE`
```cpp
template<class T>
class Array {
public:
    explicit Array(size_t n, const T& value = T{}) { /*...*/ }

    template<Iterator>
    Array(Iterator begin, Iterator end) { /*...*/ }

private:
    T* data;
};

Array<int> ar(5, 3);
```
Вроде на первый взгляд всё хорошо, но это не так. При вызове конструктора для `ar(5, 3)` ккомпилятору сложно понять: а какой именно конструктор звать-то? Внезапно, выясняется, что конструктор, принимающий итераторы будет предпочтительнее, ибо в нём не нужно кастовать `5` к `size_t`.

Но как это исправить?

На помощь приходит `Substitution failure is not an error` (хотя вы могли подумать о концептах, но они появились совсем недавно). Если при попытке подставить тип в шаблон функции при инстанцировании получается невалидный заголовок функции, то ошибки компиляции не происходит, функция просто выбрасывается из overload set.
```cpp
template<
    class Iterator,
    class Dummy = std::enable_if_t<!std::is_arithmetic_v<Iterator>>
>
Array(Iterator begin, Iterator end) { /*...*/ }
```

А как работает `std::enable_if_t`? Ну примерно так:
```cpp
template<bool Cond, typenmae T = void>
struct EnableIf;

template<typename T>
struct EnableIf<true, T> {
    using type = T;
}

template<typename T>
struct EnableIf<false, T> {
}

template<bool Cond, typenmae T = void>
using EnableIfT = EnableIf<Cond, T>::type;
```

А куда вообще совать `std::enable_if_t`?
- в шаблонный аргумент
```cpp
// (1)
template<typename T, typename = std::enable_if_t<HakKek<T>::value>>
void WithKek();

// (2)
// NTTP
template<typename T, typename = std::enable_if_t<HakKek<T>::value>* = nullptr>
void WithKek();
```
- в возвращаемое значение
```cpp
// (3)
template<typename T>
std::enable_if_t<HasKek<T>::value, T> WithKek();
```
- в аргументы функции
```cpp
// (4)
template<typename T>
T WithKek(std::enable_if_t<HasKek<T>::value*> = nullptr);
```
Ну лучше всего выбирать либо первый, либо третий варианты.

`SFINAE` очень сложно работает на методах шаблона класса:
```cpp
template<typename T>
class UniquePtr {
public:
    template<typename = std::enable_if_t<!std::is_same_v<T, void>>>
    T& Unref();  // CE
};

template<typename T>
class UniquePtr {
public:
    template<typename U = T, typename = std::enable_if_t<!std::is_same_v<U, void>>>
    U& Unref();  // OK
};
```
Такая штука может возникнуть при реализации `unique_ptr`, например, когда потребуется вернуть ссылку на `void`, что в общем-то невозможно.

Важно запомнить, что в `SFINAE` в `std::enable_if_t` надо использовать только те аргументы, которые шаблонны для данной конкретной функции, не определённой где-то выше.

## Концепты
Но жить становится проще. Концепты решают почти все проблемы использования `SFINAE`:
```cpp
template<typename T>
class UniquuePtr {
public:
    template<typename U = T>
    requires (!std::is_same_v<T, void>)
    U& Unref();  // OK
};
```

Можно написать свой концепт:
```cpp
template<typename T>
concept HasSize = requires(const T& value) {
    value.size();
}

static_assert(HasSize<std::vector<int>>);
static_assert(!HasSize<int>);
```
Или так:
```cpp
template<typename T>
concept HasSize = requires(const T& value) {
    { value.size() } -> std::convertible_to<size_t>;
}
```
Кроме того, можно писать так:
```cpp
template<typename T>
class UniquePtr {
public:
    template<typename U = T>
    requires requires(const T& value) {
        { value.size() } -> std::convertible_to<size_t>;
    }
    U& Unref() {
        if constexpr (requires(const U& value) { value.size(); }) {
        }

        throw 1;
    }
};
```
Это ОЧЕНЬ некрасиво, не надо так писать!


## `constexpr` since C++11
Можно писать функции, вычисляемые на этапе компиляции:
```cpp
constexpr int Fact(int n) {
    return n == 0 ? 1 : n * Fact(n - 1);
}

// Compile time
static_assert(Fact(8) == 40320);
std::array<int, Fact(5)> arr;

// Runtime
int n;
std::cin >> n;
std::cout << Fact(n);
```

Начиная с C++14 появилась возможность писать циклы:
```cpp
constexpr int Fact(int n) {
    int res = 1;
    for (int i = 1; i <= n; ++i) {
        res *= i;
    }
    return res;
}
```
- в функции запрещены `goto`, `try`, объявление типов. Всё остальное более менее можно.
- `constexpr` у типа не означает, что метод `const`
- много `constexpr` в `std`

Начиная с C++20 появилась возможность использовать `compile-time` аллокаторы: `std::vector`, `std::string` в `constexpr`-функциях.

`consteval` (???)
