# Лекция №4 (30/09/2025). Типы, шаблоны
## Типы
- ограничивает возможные операции над объектом
- придают понятную семинатику объекту
- есть много встроенных типов в сам язык

Из типов можно собирать другие типы, комбинируя их
```cpp
struct TwoInts {
    int64_t hi;
    int64_t lo;
};
```
Эта структура не обладает какими-либо инвариантами. 

Если инвариант всё-таки есть, хотя бы один, то заводится класс:
```cpp
class TwoInts {
public:
    TwoInts() : hi_(0), lo_(0) {
    }

    int64_t GetHigh() const {
        return hi_;
    }
    int64_t GetLow() const {
        return lo_;
    }

private:
    int64_t hi_;
    int64_t lo_;
};
```

Можно добавлять методы.

Любой тип имеет ненулевой размер в байтах (`char`). Даже пустой:
```cpp
struct Empty {};
static_assert(sizeof(Empty) == 1);
```
Это из-за того, что в C++ все объекты одного типа имеют разные адреса, нет `zero sized types` (привет, Rust).

Однако, в стандарте есть лазейка, позволяющая хранить пустые типы без лишней памяти (привет, `compressed pair`, то есть `Empty Base Optimization`).

В C++20 появилась более безопасная штука, а именно - `[[no_unique_address]]`. По сути своей она делает то же, что и `EBO`, но аккуратнее.

Например, `std::allocator<T>` не занимает места. Он в целом сам по себе имеет по сути только методы в своей реализации: аллоцирует и деаллоцирует память.

Компилятор может не добавлять пустые базы в layout типа, если это ничему не протвирочит:
```cpp
struct IntWithEmpty : Empty {
    int value;
};
static_assert(sizeof(IntWithEmpty) == sizeof(int));  // not guaranteed
```

По одному адресу в памяти может лежать куча объектов разных типов. При этом всё равно нельзя иметь объекты одного типа с одинаковыми адресами:
```cpp
struct Empty {};

struct TwoEmpty : Empty {
    Empty anotherEmpty;
};
static_assert(sizeof(TwoEmpty) == 2);

struct Proxy1 : Empty {};
struct Proxy2 : Empty {};
struct TwoEmptyBase : Proxy1, Proxy2, {};
static_assert(sizeof(TwoEmptyBase) == 2);
```

Не смотря на то, что `[[no_unique_address]]` явно описан в стандарте, тем не менее использовать его не стоит в виду странного поведения в некоторых ситуациях.


## Шаблоны
С шаблонами мы уже сталкивались: это параметризованная типами/значениями сущность (класс, функция, перемеренная):
```cpp
template<typename T1, typename T2>  // Между class и typename никакой разницы, но лучше typename
struct pair {
    T1 first;  // Use of T1
    T2 second;  // Use of T2
};

int x = 0;
pair<int, int> p(x, x);
pair<int, int&> p(x, x);  // Because why not

x = std::max<int32_t>(0, x);
```

Бывают шаблоны переменных: редко используемая фича, но иногда полезно:
```cpp
template<int A, int B>
constexpr int Sum = A + B;
```

#### NTTP (non-type tempaplate parametr)
Бывают значения в качестве шаблонных параметров:
```cpp
template<typename T, size_t Count>
class Array {
    T buf[Count];
};
```

Все аргументы шаблона подставляются на этапе компиляции! До этого момента на нас вряд ли наругаются. То есть, условно говоря, вот такой код будет валидным:
```cpp
template<typename T, size_t Count>
class Array {
    void Foo() {
        T value{42, 42, 42};
        value.unknown_method();
    }

    T buf[Count];
};
```
Хотя мы не знаем, есть ли у `T` конструктор для трёх значений, метод `unknown_method()`, тем не менее такой код будет корректным (до момента компиляции, конечно).

С C++20 сильно расширили возможные типы NTTP. С появлением стандарта нашлись любители, которые написали библиотеку регулярных выражений полностью на шаблонах:
```cpp
template<util::fixed_string regex>
class StaticRegularExpression;

StaticRegularExpression<"([0-9a-f]{8}-){3}([0-9a-f]{8}-)"> regex;

// util::fixed_string нужно реализовывать самим
```
Красиво, но безумно медленно компилируется, очень сложно дебажить, непонятно, зачем нужно, но красиво.

Шаблонные аргументы и типы могут выводиться (в смысле вычисляться):
```cpp
template<typepname T>
T inc(T a) { return a + 1; }

// Компилятор автоматически понимает, что нужен int
inc(5);  // call inc<int>

int x = -1;
x = std::max(0, x);  // call std::max<int>();

template<int I>
int foo(std::integral_constant<int, I> _unused) {
    return I * I;
}

void bar() {
    foo(std::integral_constant<int, 5>{});  // call foo<5>
}

// Даже в таком сложном коде компилятор пониимает, что нужно подставить,
// а не заставляет пользователя явно прописывать тип при вызове foo()
```

Однако, иногда типы не вычисляются:
```cpp
uint32_t x = 0;
x = std::max(0, x);  // long compiler error
```

#### CTAD (class templation deduction guide)
В том числе и для классов порой работает вычисление типов:
```cpp
std::vector v{1, 2, 3};  // it's OK
```

Или так:
```cpp
template<typename T, typename U>
class Pair {
public:
    Pair(T lhs, U rhs) : lhs_(std::move(lhs)), rhs_(std::move(rhs)) {
    }

private:
    T lhs_;
    U rhs_;
};

int main() {
    pair p{0, "Hello"};  // CE before C++17, pair<int, const char*> after C++17
}
```

Однако, в некоторых случаях в классах это работает не очень хорошо. Например:
```cpp
template<typename T, typename U>
class Pair {
public:
    Pair(const T& lhs, const U& rhs) : lhs_{lhs}, rhs_{rhs} {
    }

private:
    T lhs_;
    U rhs_;
};

int main() {
    pair p{0, "Hello"};  // U вычисляется, как char[6], однако, на этапе инициализации rhs мы получаем ошибку
}
```

Но есть ужасная штука под названием `class template deduction guide`, она сильнее. Это способ направить компилятор на правильное вычисление типа:
```cpp
template<typename T1, typename T2>
struct pair {
    pair(const T1& f, const T2& s) : first(f), second(s) {
    }
    T1 first;
    T2 second;
};

template<typename T1, typename T2>
pair(const T1&, const T2&) -> pair<const T1&, const T2&>;

pair p{0, 1};  // pair<const int&, const int&>
```
Мы явно указываем, как из заданного конструктора вывести аргументы шаблона. Просто подсказываем, какие типы подставить. `Deduction Guide` пишется для конструкторов. Штука крайне полезная, ибо обычного вычисления типов не хватает зачастую. Тем более крайне полезна она при работе с ссылками.
