# Лекция №3 (25/09/2025). Move-семантика. Продолжение
## Продолжаем про move-семантику
### Немного об операторе присваивания-перемещения
Ещё раз упоминание о контракте между вызывающей стороной и вами:
- вызывающая сторона обязуется позвать деструктор
- вы обязуетесь оставить объект в консистентном состоянии для вызова деструктора

С появлением `move-семантики` правило трёх `rule-of-3` превратилось в правило пяти `rule-of-5`. Это означает, что если вы реализуете хотя бы один из конструкторов, или операторов присваивания, или деструктор, то вы обязаны реализовать и все остальные из перечисленных здесь специальных функций.

Вот, например, правильная реализация оператора присваивания-перемещающего:
```cpp
Holder& operator=(Holder&& other) {
    if (this == &other) {
        return *this;
    }

    delete[] ptr_;
    size_ = 0;

    // прикольная функция, которая пригождается при реализации
    // перемещащего оператора присваивания
    // присвает новое значение другому и зануляет указатель текущий
    ptr_ = std::exchange(other.ptr_, nullptr);
    // то есть ptr_ = other.ptr_
    // а other.ptr_ = nullptr

    std::swap(ptr_, other.ptr_);
    std::swap(size_, other.size_);

    return *this;
}
```


### Немного о правиле 5
Хорошее правило: либо вы указываете все конструкторы/операторы (rule of 5), либо ничего (rule of 0):
1. Копирующий конструктор
2. Копирующий оператор присваивания
3. Перемещающий конструктор
4. Перемещающий оператор присваивания
5. Деструктор

Там, где можно, ставьте `= default` или `= delete`. Подавляющее большинство случаев.

Это реально маргинальный случай, когда в 2025 году требуется самому писать своё копирование. Пользуйтесь дефолтными определениями.

Всегда, когда вы пишите оператор присваивания перемещающий или конструктор перемещения, пишите ключевое слово `noexcept`:
```cpp
Holder(Holder&&) noexcept = default;
Holder& operator=(Holder&&) noexcept = default;
```
Например, если у вас вектор объектов вашего класса, то без `noexcept` он будет копировать ваши объекты, а не перемещать.


### Чудеса реализации std::move
```cpp
template<typename T>
std:::remove_reference_t<T>&& Move(T&& ref) {
    return static_cast<std::remove_reference_t<T>&&>(ref);
}
```
Мы хотим, чтобы функция работала и с временными объектами (rvalue), и с lvalue. Но как это сделать? Если коротко, то без `std::remove_reference_t<>` это работать не будет. В данной реализации `Move` получает не `rvalue`-ссылку, как может показаться, а универсальную ссылку. !!!Важно, что `rvalue` принимаемый, здесь шаблонный!!! Универсальная ссылка толька та, которая шаблонизирована. Работает же это примерно так:
- если передаём условно говоря T, то всё просто преобразуется в `rvalue`
- если передаём lvalue `T&`, то получается ситуация, когда передано `(T& && ref)`. Компилятор смотрит и видит `lvalue` и `rvalue`. `lvalue` по правилам языка сильнее, а потому остаётся именно `lvalue`, и всё работает корректно. 

Но ещё раз! Без `std::remove_reference_t<>` это не сработает!


### Немного о константности
Никогда не делайте `move` на `const` объекты. `const` от типа может не отлипнуть:
```cpp
class Annotation {
public:
    explicit Annotation(const std::string text) : value_(std::move(text)) {
    }

    // we want to call string(string&&)
    // text is const
    // std::move(text) is const std::string&&
    // we called string(const std::string&)
    // it is a perf issue.

private:
    std::string value_;
};
```
Константная `rvalue`-ссылка самый бесполезный тип, который можно придумать. По факту по возможностям она вообще никак не отличается от константной `lvalue`-ссылки. В языке эта конструкция существует просто для симметрии.


### Последние слова про move-семантику
Все доступные комбинации:
```cpp
void f1(std::string& s);
void f2(const std::string& s);
void f3(std::string&& s);
void f4(const std::string&& s);

std::string& s("Hi!");  // lvalue
const std::string& cs();  // const lvalue

f1(s);  // OK
f1(cs);  // ERROR
f1(std::move(s));  // ERROR
f1(std::move(cs));  // ERROR

f2(s);  // OK
f2(cs);  // OK
f2(std::move(s));  // OK
f2(std::move(cs));  // OK

f3(s);  // ERROR
f3(cs);  // ERROR
f3(std::move(s));  // OK
f3(std::move(cs));  // ERROR

f4(s);  // ERROR
f4(cs);  // ERROR
f4(std::move(s));  // OK
f4(std::move(cs));  // OK
```


### Ну и наконец самая последняя часть move-семантики
Каждый раз, когда вы хотите передать `rvalue` ссылку дальше, нужно прописать `std::move`:
```cpp
class Annotation {
public:
    explicit Annotation(std::string&& text) : value_(text) {
    }

    // Здесь мы передаём lvalue, оно не отделимо от text
    // Поэтому происходит копирование
    // Если мы реально хотим работать с rvalue, то необходимо прописать std::move(text)

private:
    std::string value_;
};
```


### Perfect forwarding (всё ещё move-семантика)

`std::forward` нужен, чтобы просунуть принятое значение в следующую функцию. Пример:
```cpp
template<typename T, typename Arg>
void BenchVectorConstructor(Arg&& arg) {
    auto start = std::clock();
    std::vector<T> vec(std::forward<Arg>(arg));
    std::vector<T> vec(static_cast<Arg&&>(arg));
    auto end = std::clock();
}
```
Здесь мы принимаем универсальную ссылку, чтобы замерить время создания вектора. Но недостаточно просто написать `std::vector<T> vec(arg)`. Здесь будет копирование, мы померием вообще не то. Но в то же время нельзя написать и `std::vector<T> vec(std::move(arg))`, потому что, если пользователь дал нам `lvalue`, то мы просто украдём у него объект. Ну странная ситуация получается. Для этого и существует `std::forward`. Если нам передают `lvalue`, то он он работает с `lvalue`, если передают `rvalue`, то будет вызван `std::move`.

Но важно, в случае `std::forward` должна быть универсальная ссылка!

Ну а по сути `std::forward` нужен для перенаправления аргументов в базовый класс:
```cpp
template<typename U>
Status0r(U&& v) : Base(std::forward<U>(v)) {
}
```


### Ref qualifiers (ну теперь точно последнее про move-семантику)
Мы уже писали перегрузки методов по константности:
```cpp
class Foo {
public:
    int& Get() {
        return value_;
    }

    const int& Get() const {
        return value_;
    }
};
```
Но можно ли перегружать не только по константности, но и по rvalue/lvalue?

Оказывается, что можно:
```cpp
struct Foo {
    int& Get() & { return value_; }

    const int& Get() const& { return value_; }

    int&& Get() && { return std::move(value_); }

    const int&& Get() const&& { return std::move(value_); }
    //  ^^^             ^^^
    // возвращаемый    принимаемый
    //    тип              тип
};
```

А как их звать?
```cpp
Foo{}.Get();  // call Get() &&

Foo foo;
foo.Get();  // call Get() &
std::as_const(foo).Get();  // call Get() const&
sttd::move(foo).get();  // call Get() &&
```


### Deducing this
Выглядит не очень красиво, а потому в *C++23* появилась такая штука (похоже на Rust; имхо, я первое чё вспомнил, так это ООП в Python):
```cpp
SearchIndex Finish(this auto&& self) {  // this SearchIndex&& self, и он обязательно идёт первым
    if (std::exchange(finished_, true)) {
        throw std::runtime_error{"Double finish!"};
    }
    auto index = SearchIndex{std::move(self.inverted_index_)};
    self.~SearchIndexBuilde();  // вместо this->~SearchIndexBuilder()
    return index;
}

// Вместо того, чтобы писать SearchIndex Finish() && {}
```

До C++23 постоянно приходилось копипастить эти функции. Но с появлением `this auto&&`, можно шаблонизировать функцию, и тогда `this auto&&` будет восприниматься, как универсальная ссылка. Ну а тогда можно будет взависимости от типа кастомить функцию внутри одной реализации, избегая бесконечного копирования кода.

Для выполния задачки `compressed_pair` понадобится знание механизма `empty base optimization`
