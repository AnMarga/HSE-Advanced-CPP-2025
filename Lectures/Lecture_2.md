# Лекция №2 (16/09/2025). Динамическая память. Move-семантика
## Заканчиваем рассмотрение видов памяти
- automatic storage duration (жаргон "на стеке")
- static storage duration (жаргон "глобальный", глобальные переменные)
- dynamic storage duration (жаргон "в куче", классический new)
- thread-local storage duration (C++11, после многопоточки; будем обсуждать только через 10 лекций)

### Динамическая память
- выделяется и освобождается руками
- объекты живут столько, сколько напишет программист
- low-level сырые `operator new`, `malloc` (вовзращают указатель на блоки сырой памяти)
- high-level типизированный `new` expression (`new int`, `new char[32]`)
- обёртки вроде `std::vector`, `std::string`

Примеры использования:
```cpp
int kek* = new int{42};
delete kek;

int* array = new int[100];
delete[] array;

std::unique_ptr<int> ptr(new int{19});

// Calls operator new(sizeof(int) * 100)
std::vector<int> v(100);

std::unordered_map<int, int> x;
// Calls new std::pair<const int, int>(1, 2);
x[1] = 2;
```

```cpp
int* x = new int();
delete x;

int* arr = new int[100];
delete[] arr;
// delete arr - UB! Имеется в виду если без [] прописать

new int();
// Leak, but not UB. Memory leaks are safe
```
- за каждым `new` должен идти ровно один корректный `delete`
- std::vector, std::unique_ptr, другие контейнеры делают это в деструкторе
- Если нет `new`, значит нет и `delete`

Случай с `std::unique_ptr` немного некрасивый, в том плане, что мы зовём `new`, но не зовём `delete`. Поэтому специально для этого была реализована функция `std::make_unique`, которая под капотом зовёт `new`:
```cpp
// Вместо std::unique_ptr<int> ptr(new int{19}); пишем это
std::unique_ptr<int> ptr = std::make_unique(int(19));
```

`new int[]` выделяет сырые байти памяти, а затем в каждой ячейке вызывает конструктор. `delete[]` делает противоположное: пробегает по всем ячейкам, вызывая деструкторы, а затем уже освобождает саму память.

Вот эта штука - UB:
```cpp
#include <memory>

int main() {
    // [size[ ][ ][ ]]
    //      ^^^
    // Указатель смотрит на первый элемент, а размер, например игнорирует
    // То есть в итоге высвобождается не вся память
    // P.S. это просто пример, модель может быть и иной, но обычно это так и выглядит:
    // размер массива, потом сами элементы
    std::unique_ptr<int> ptr(new int[100]);
}
```

А вот уже корректный пример:
```cpp
std::unique_ptr<int[]> ptr(new int[100]);
```
Но вообще у `std::unique_ptr` есть второй шаблонный параметр: `deleter`. Через него можно закастомить `delete`.
```cpp
std::unique_ptr<int[], std::default_delete<int[]>> ptr(new int[100]);
```

На C нет удобных инструментов для работы с памятью. Управление ресурсами это каждый раз:
- `malloc` для выделения памяти
- `goto err` обработка каждой ошибки
- `free` для освобождения памяти

При таких вводных написание кода на C было и есть - боль. 

Рекомендации/приципы при работе с динамической памятью:
- не используйте `malloc`, `operator new`, `new expression` никогда!
  В основном они используются при написании low-level кода, например, при реализации собственных контейнеров.
  Но даже там лучше использовать `std::allocator<TypeName>`, и это даже рекомендуется, и считается хорошей практикой.
- используйте `std::unique_ptr` и `std::make_unique`, контейнеры
- использование `new` - моветон, так плохо писать код; скорее всего будут ошибки:
    - Проверяйте свой код при помощи Address Sanitizer


### Автоматическая память
```cpp
int x;
```
Автоматически выделяет и освобождает память, никакой ручной работы.

Почти всегда здесь память выделяется на стеке.

Автоматическая память достаточно маленькая (2, 4, 8 Мб), поэтому используйте её для хранения указателей на динамическую память. Что-то большое хранить в автоматической памяти не получится.


### Статическая память
```cpp
static int y;
```
Глобальные переменные, переменные, которые живут на протяжении всего исполнения программы.

Меняющиеся глобальные переменные зло, но как и везде есть исключения. Сами по себе глобальные переменные занимают место в выходном бинарном файле.

Константы, конечно, заводить можно.



## Move-семантика
### Категория значения
- Каждое выражение в C++ характеризуется типом и категорией значения (`value category`)
- Всего есть три (на самом деле гораздо больше, порядка 7-8) различных категории значения:
    - `lvalue` (left hand side value)
    - `xvalue` (eXpiring value)
    - `prvalue` (pure rvalue)
- `xvalue` и `prvalue` похожи, для них определена категория `rvalue` (right hand side value)
- Интуиция: исторически назвали так, потому что `lvalue` может быть на левой стороне оператора присваивания, а `rvalue` - нет

Примеры:
```cpp
int i = 1;                          // i is lvalue, 1 is rvalue (literal constant)
int* y = &i;                        // we can take addresses from lvalue as they have location
1 = i;                              // ERROR: 1 is rvalue, can not be on the left
y = &42;                            // ERROR: 42 is rvalue, no location
const char(*ptr)[5] = &"abcd";      // string literal is lvalue (!)
std::cout << "Hello, " << name;     // std::ostream& operator<<(std::ostream&, const char*);

char arr[20];
arr[10] = 'z';                      // arr[10] returns lvalue reference

int a, b;
(a, b) = 5;                         // same as "b = 5;"

const char* p = ...;
*p++ = 'a';
```
- У `lvalue` можно брать адреса. У них есть конкретное местоположение в памяти.
- Важно отметить, что строковые литералы - это `lvalue`, у них можно взять адрес:
    - Небольшое пояснение: дело в том, что для строк выделяется место в бинарнике специально (ну Сквор так сказал), поэтому мы знаем их местоположение. А вот численные константы создаются, что называется, `in-place`
- На самом деле `lvalue` не всегда может быть слева, как подсказывает интуиция. Например, мы не можем ничего присвоить строке. Поэтому правильнее звучало бы определение о том, что у `lvalue` есть адрес

Ещё пачка примеров:
```cpp
int GetValue() {
    return 42;
}

int& GetGlobal() {
    static int j;
    return j;
}

GetValue() = 1;     // ERROR: GetValue is not lvalue, it is temporary returned from function
GetGlobal() = 5;    // ok, int& is lvalue reference, it has location
```

И ещё:
```cpp
// ok
int a = 42;
int& a_ref = a;  // a - lvalue
++a_ref;
a_ref = 5;

// not ok
int& b_ref = 10;  // 10 - константа
```

И ещё:
```cpp
void foo(int& x) {
}

foo(10);  // not ok
```
```cpp
void foo_const(const int& x) {
}

foo_const(10);  // ok
// по факту создаст новый объект, когда 10 передадут в функцию, и он уже будет иметь адрес в памяти
```

Но что же происходит на самом деле?
#### Temporary lifetime extension
Продление времени жизни временного объекта:
```cpp
// Why?
// Well, it's a mistake and not at the same time.

// the following...
const int& ref = 10;

// ... would translate to:
int __internal_unique_name = 10;
const int& ref = __internal_unique_name;

// Without this rule we wouldn't do the things like:
class T;  // defined somewhere
T f();
void g(const T& x);

g(f());
const T& x = f();  // also works, то же самое, что и "T x = f()", так что лучше писать так, без выпендрёжа
```
Но вообще говоря не рекомендуется использовать эту фичу, ибо она довольно-таки неявная и неочевидная.

### Move-семантика
До C++11:
```cpp
void move_string(std::string& s, std::string& f) {
    s.swap(f);
    f.clear();
}
```

Но в C++11 появились `rvalue references`. Это самая важная фича C++11. Выделяется она своими двумя амперсандами: `&&`. И нет, это не ссылка на ссылку, это просто синтаксис, чтобы различать `lvalue` и `rvalue` ссылки.

`rvalue`-ссылка - это контракт между вами и пользователем (вызывающей стороной) вашего класса (функции), который говорит, что вызывающая сторона вам явно передаёт объект, из которого вы можете забрать все внутренности себе. Он больше не нужен вызывающей стороне, единственное что сделает эта сторона - позовёт деструткор.

Вот, например, реализация копирования:
```cpp
Holder(Holder&& other) {
    data_ = other.data_;
    size_ = other.size_;
    other.data_ = nullptr;
    other.size_ = 0;
}
```

Оператор присваивания (перемещающий):
```cpp
Holder& operator=(Holder&& other) {
    if (this == &other) return *this;

    delete data_[];

    data_ = other.data_;
    size_ = other.size_;

    other.data_ = nullptr;
    other.size_ = 0;

    return *this;
}
```

Можно превращать `lvalue` ссылки в `rvalue` ссыслки через `std::move`:
```cpp
Holder h1(1000);
Holder h2(std::move(h1));
```
Это просто каст (`static_cast<Holder&&>`) к `rvalue` ссылке.
