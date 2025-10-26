#pragma once

#include <iterator>
#include <type_traits>
#include <utility>

// ----------------- IteratorRange -----------------
template <class Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end) : begin_(begin), end_(end) {}

    Iterator begin() const {  // NOLINT
        return begin_;
    }

    Iterator end() const {  // NOLINT
        return end_;
    }

private:
    Iterator begin_;
    Iterator end_;
};

// ----------------- Range -----------------
template <typename T>
class RangeImpl {
public:
    class Iterator {
    public:
        using ValueType = T;
        using Reference = T;
        using Pointer = void;
        using IteratorCategory = std::input_iterator_tag;
        using DifferenceType = std::ptrdiff_t;

        using value_type = ValueType;
        using reference = Reference;
        using pointer = Pointer;
        using iterator_category = IteratorCategory;
        using difference_type = DifferenceType;

        Iterator(T cur, T step, T end) : cur_(cur), step_(step), end_(end) {}

        T operator*() const { return cur_; }

        Iterator& operator++() {
            cur_ += step_;
            return *this;
        }

        bool operator!=(const Iterator&) const {
            if (step_ > 0) {
                return cur_ < end_;
            } else {
                return cur_ > end_;
            }
        }

    private:
        T cur_;
        T step_;
        T end_;
    };

    RangeImpl(T from, T to, T step) : from_(from), to_(to), step_(step) {}

    Iterator begin() const { return Iterator(from_, step_, to_); }  // NOLINT
    Iterator end() const { return Iterator(to_, step_, to_); }      // NOLINT

private:
    T from_;
    T to_;
    T step_;
};

// Range(stop)
template <typename T>
auto Range(T to) {
    static_assert(std::is_integral_v<T>, "Range requires integral type");
    return RangeImpl<T>(T(0), to, T(1));
}

// Range(start, stop)
template <typename T1, typename T2>
auto Range(T1 from, T2 to) {
    using T = std::common_type_t<T1, T2>;
    return RangeImpl<T>(static_cast<T>(from), static_cast<T>(to), static_cast<T>(1));
}

// Range(start, stop, step)
template <typename T1, typename T2, typename T3>
auto Range(T1 from, T2 to, T3 step) {
    using T = std::common_type_t<T1, T2, T3>;
    return RangeImpl<T>(static_cast<T>(from), static_cast<T>(to), static_cast<T>(step));
}

// ----------------- Zip -----------------
template <typename It1, typename It2>
class ZipIterator {
public:
    using Ref1 = typename std::iterator_traits<It1>::reference;
    using Ref2 = typename std::iterator_traits<It2>::reference;
    using ValueType = std::pair<Ref1, Ref2>;

    ZipIterator(It1 cur1, It1 end1, It2 cur2, It2 end2)
        : cur1_(cur1), end1_(end1), cur2_(cur2), end2_(end2) {}

    ValueType operator*() const { return {*cur1_, *cur2_}; }

    ZipIterator& operator++() {
        ++cur1_;
        ++cur2_;
        return *this;
    }

    bool operator!=(const ZipIterator&) const {
        return cur1_ != end1_ && cur2_ != end2_;
    }

private:
    It1 cur1_;
    It1 end1_;
    It2 cur2_;
    It2 end2_;
};

template <typename C1, typename C2>
auto Zip(const C1& a, const C2& b) {
    using It1 = decltype(std::begin(a));
    using It2 = decltype(std::begin(b));
    using ZIt = ZipIterator<It1, It2>;
    ZIt begin_it(std::begin(a), std::end(a), std::begin(b), std::end(b));
    ZIt end_it(std::end(a), std::end(a), std::end(b), std::end(b));
    return IteratorRange<ZIt>(begin_it, end_it);
}

// ----------------- Group -----------------
template <typename It>
class GroupIterator {
public:
    using IteratorType = It;
    using ValueType = IteratorRange<It>;

    GroupIterator(It cur, It end) : cur_(cur), end_(end) {}

    ValueType operator*() const {
        It group_begin = cur_;
        It it = cur_;
        if (it == end_) {
            return IteratorRange<It>(it, it);
        }
        const auto& key = *group_begin;
        ++it;
        while (it != end_ && *it == key) {
            ++it;
        }
        return IteratorRange<It>(group_begin, it);
    }

    GroupIterator& operator++() {
        if (cur_ == end_) {
            return *this;
        }
        const auto& key = *cur_;
        do {
            ++cur_;
        } while (cur_ != end_ && *cur_ == key);
        return *this;
    }

    bool operator!=(const GroupIterator& other) const { return cur_ != other.cur_; }

private:
    It cur_;
    It end_;
};

template <typename Container>
auto Group(const Container& c) {
    using It = decltype(std::begin(c));
    using GIt = GroupIterator<It>;
    GIt begin_it(std::begin(c), std::end(c));
    GIt end_it(std::end(c), std::end(c));
    return IteratorRange<GIt>(begin_it, end_it);
}
