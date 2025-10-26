#pragma once

#include <cassert>
#include <cstddef>
#include <iterator>
#include <stdexcept>

// Узел для intrusive list
class ListHook {
public:
    ListHook() : prev_(nullptr), next_(nullptr) {
    }
    ListHook(const ListHook&) = delete;
    ListHook& operator=(const ListHook&) = delete;
    ListHook(ListHook&&) = delete;
    ListHook& operator=(ListHook&&) = delete;

    ~ListHook() {
        if (IsLinked()) {
            Unlink();
        }
    }

    bool IsLinked() const {
        return prev_ != nullptr && next_ != nullptr;
    }

    void Unlink() {
        assert(IsLinked());
        prev_->next_ = next_;
        next_->prev_ = prev_;
        prev_ = nullptr;
        next_ = nullptr;
    }

private:
    ListHook* prev_;
    ListHook* next_;

    template <class T>
    friend class List;

    void LinkBefore(ListHook* other) {
        assert(!IsLinked());
        prev_ = other->prev_;
        next_ = other;
        other->prev_->next_ = this;
        other->prev_ = this;
    }
};

template <typename T>
class List {
public:
    class Iterator {
        using IteratorTag = std::bidirectional_iterator_tag;

    public:
        typedef T value_type;
        typedef ptrdiff_t difference_type;
        typedef T* pointer;
        typedef T& reference;
        typedef IteratorTag iterator_category;

        Iterator(ListHook* ptr) : ptr_(ptr) {
        }

        Iterator& operator++() {
            ptr_ = ptr_->next_;
            return *this;
        }
        Iterator operator++(int) {
            Iterator tmp(*this);
            ++(*this);
            return tmp;
        }

        Iterator& operator--() {
            ptr_ = ptr_->prev_;
            return *this;
        }
        Iterator operator--(int) {
            Iterator tmp(*this);
            --(*this);
            return tmp;
        }

        reference operator*() const {
            return static_cast<T&>(*ptr_);
        }
        pointer operator->() const {
            return &static_cast<T&>(*ptr_);
        }

        bool operator==(const Iterator& rhs) const {
            return ptr_ == rhs.ptr_;
        }
        bool operator!=(const Iterator& rhs) const {
            return !(*this == rhs);
        }

    private:
        ListHook* ptr_;
    };

    List() {
        dummy_.next_ = &dummy_;
        dummy_.prev_ = &dummy_;
    }

    List(const List&) = delete;
    List& operator=(const List&) = delete;

    List(List&& other) noexcept {
        if (other.IsEmpty()) {
            dummy_.next_ = dummy_.prev_ = &dummy_;
        } else {
            dummy_.next_ = other.dummy_.next_;
            dummy_.prev_ = other.dummy_.prev_;
            dummy_.next_->prev_ = &dummy_;
            dummy_.prev_->next_ = &dummy_;
            other.dummy_.next_ = other.dummy_.prev_ = &other.dummy_;
        }
    }

    List& operator=(List&& other) noexcept {
        if (this != &other) {
            Clear();
            if (other.IsEmpty()) {
                dummy_.next_ = dummy_.prev_ = &dummy_;
            } else {
                dummy_.next_ = other.dummy_.next_;
                dummy_.prev_ = other.dummy_.prev_;
                dummy_.next_->prev_ = &dummy_;
                dummy_.prev_->next_ = &dummy_;
                other.dummy_.next_ = other.dummy_.prev_ = &other.dummy_;
            }
        }
        return *this;
    }

    ~List() {
        Clear();
    }

    bool IsEmpty() const {
        return dummy_.next_ == &dummy_;
    }

    size_t Size() const {
        size_t size = 0;
        for (auto* it = dummy_.next_; it != &dummy_; it = it->next_) {
            ++size;
        }
        return size;
    }

    void Clear() {
        while (!IsEmpty()) {
            PopFront();
        }
    }

    void PushBack(T* elem) {
        elem->LinkBefore(&dummy_);
    }
    void PushFront(T* elem) {
        elem->LinkBefore(dummy_.next_);
    }

    T& Front() {
        if (IsEmpty()) {
            throw std::runtime_error("List is empty");
        }
        return static_cast<T&>(*dummy_.next_);
    }
    const T& Front() const {
        if (IsEmpty()) {
            throw std::runtime_error("List is empty");
        }
        return static_cast<const T&>(*dummy_.next_);
    }

    T& Back() {
        if (IsEmpty()) {
            throw std::runtime_error("List is empty");
        }
        return static_cast<T&>(*dummy_.prev_);
    }
    const T& Back() const {
        if (IsEmpty()) {
            throw std::runtime_error("List is empty");
        }
        return static_cast<const T&>(*dummy_.prev_);
    }

    void PopFront() {
        if (IsEmpty()) {
            throw std::runtime_error("List is empty");
        }
        dummy_.next_->Unlink();
    }
    void PopBack() {
        if (IsEmpty()) {
            throw std::runtime_error("List is empty");
        }
        dummy_.prev_->Unlink();
    }

    Iterator Begin() {
        return Iterator(dummy_.next_);
    }
    Iterator End() {
        return Iterator(&dummy_);
    }

    Iterator IteratorTo(T* element) {
        return Iterator(static_cast<ListHook*>(element));
    }

private:
    ListHook dummy_;
};

// free functions for range-based for
template <typename T>
typename List<T>::Iterator begin(List<T>& list) {  // NOLINT
    return list.Begin();
}

template <typename T>
typename List<T>::Iterator end(List<T>& list) {  // NOLINT
    return list.End();
}
