#pragma once

#include <cstddef>
#include <initializer_list>
#include <algorithm>
#include <stdexcept>
#include <cstring>

class Deque {
public:
    // 128 int = 128 * 4 = 512 bytes
    const size_t BLOCK_SIZE = 128;

    // Default ctor
    Deque()
        : blocks_(nullptr),
          capacity_buf_(0),
          size_buf_(0),
          size_(0),
          start_block_(0),
          start_offset_(0) {}

    // Count ctor (fill zeros)
    explicit Deque(size_t n)
        : blocks_(nullptr),
          capacity_buf_(0),
          size_buf_(0),
          size_(0),
          start_block_(0),
          start_offset_(0) {
        if (n == 0) {
            return;
        }
        // сколько блоков нужно
        size_t needed_blocks = (n + BLOCK_SIZE - 1) / BLOCK_SIZE;
        // вместимость буфера — двойной запас
        capacity_buf_ = std::max<size_t>(1, needed_blocks * 2);
        blocks_ = new int*[capacity_buf_];
        for (size_t i = 0; i < capacity_buf_; ++i) {
            blocks_[i] = nullptr;
        }

        // выделяем необходимые блоки и заполняем нулями
        for (size_t i = 0; i < needed_blocks; ++i) {
            blocks_[i] = new int[BLOCK_SIZE](0);
        }
        size_buf_ = needed_blocks;
        size_ = n;
        start_block_ = 0;
        start_offset_ = 0;
    }

    // initializer_list ctor
    Deque(std::initializer_list<int> list)
        : Deque(list.size()) {
        size_t idx = 0;
        for (int v : list) {
            // безопасно, так как Deque(size) выделил блоки
            (*this)[idx++] = v;
        }
    }

    // Copy ctor (deep copy)
    Deque(const Deque& other)
        : blocks_(nullptr),
          capacity_buf_(0),
          size_buf_(0),
          size_(0),
          start_block_(0),
          start_offset_(0) {
        if (other.size_ == 0) {
            return;
        }

        // выделим буфер указателей сопоставимого размера
        capacity_buf_ = std::max<size_t>(1, other.size_buf_ * 2);
        blocks_ = new int*[capacity_buf_];
        for (size_t i = 0; i < capacity_buf_; ++i) {
            blocks_[i] = nullptr;
        }

        // выделим и скопируем блоки в том же порядке (чтобы start_block_ = 0)
        for (size_t b = 0; b < other.size_buf_; ++b) {
            blocks_[b] = new int[BLOCK_SIZE];
            size_t other_idx = (other.start_block_ + b) % other.capacity_buf_;
            // копируем весь блок (в т.ч. неиспользуемые части — безопасно)
            std::memcpy(blocks_[b], other.blocks_[other_idx], BLOCK_SIZE * sizeof(int));
        }

        size_buf_ = other.size_buf_;
        size_ = other.size_;
        start_block_ = 0;
        start_offset_ = other.start_offset_;
    }

    // Move ctor
    Deque(Deque&& other) noexcept
        : blocks_(other.blocks_),
          capacity_buf_(other.capacity_buf_),
          size_buf_(other.size_buf_),
          size_(other.size_),
          start_block_(other.start_block_),
          start_offset_(other.start_offset_) {
        other.blocks_ = nullptr;
        other.capacity_buf_ = 0;
        other.size_buf_ = 0;
        other.size_ = 0;
        other.start_block_ = 0;
        other.start_offset_ = 0;
    }

    // Copy assignment
    Deque& operator=(const Deque& other) {
        if (this != &other) {
            Deque tmp(other);
            Swap(tmp);
        }
        return *this;
    }

    // Move assignment
    Deque& operator=(Deque&& other) noexcept {
        if (this != &other) {
            // освободим свои ресурсы
            Clear();
            delete[] blocks_;

            // заберём ресурсы у other
            blocks_ = other.blocks_;
            capacity_buf_ = other.capacity_buf_;
            size_buf_ = other.size_buf_;
            size_ = other.size_;
            start_block_ = other.start_block_;
            start_offset_ = other.start_offset_;

            other.blocks_ = nullptr;
            other.capacity_buf_ = 0;
            other.size_buf_ = 0;
            other.size_ = 0;
            other.start_block_ = 0;
            other.start_offset_ = 0;
        }
        return *this;
    }

    ~Deque() {
        Clear();            // удаляем все блоки
        delete[] blocks_;   // удаляем массив указателей
        blocks_ = nullptr;
        capacity_buf_ = 0;
    }

    // operator[]
    int& operator[](size_t i) {
        if (i >= size_) {
            throw std::out_of_range("Index out of range");
        }
        size_t total = start_offset_ + i;
        size_t block_shift = total / BLOCK_SIZE;
        size_t offset = total % BLOCK_SIZE;
        size_t block_index = (start_block_ + block_shift) % capacity_buf_;
        return blocks_[block_index][offset];
    }

    const int& operator[](size_t i) const {
        if (i >= size_) {
            throw std::out_of_range("Index out of range");
        }
        size_t total = start_offset_ + i;
        size_t block_shift = total / BLOCK_SIZE;
        size_t offset = total % BLOCK_SIZE;
        size_t block_index = (start_block_ + block_shift) % capacity_buf_;
        return blocks_[block_index][offset];
    }

    size_t Size() const { return size_; }

    void Clear() {
        if (blocks_ != nullptr && capacity_buf_ > 0 && size_buf_ > 0) {
            // удаляем только реально выделенные блоки (size_buf_ штук, начиная с start_block_)
            for (size_t i = 0; i < size_buf_; ++i) {
                size_t idx = (start_block_ + i) % capacity_buf_;
                if (blocks_[idx] != nullptr) {
                    delete[] blocks_[idx];
                    blocks_[idx] = nullptr;
                }
            }
        }
        size_ = 0;
        size_buf_ = 0;
        start_block_ = 0;
        start_offset_ = 0;
        // НЕ удаляем сам массив указателей — это делает деструктор (или EnsureCapacityBlocks при переносе)
    }

    // PushBack
    void PushBack(int value) {
        // Нужен хотя бы один буфер указателей
        if (capacity_buf_ == 0) {
            EnsureCapacityBlocks(1);
        }

        // вычислим, в каком (относительно start_block_) блоке должен лежать элемент
        size_t block_with_offset = (start_offset_ + size_) / BLOCK_SIZE; // смещение блока от start_block_
        // если нужно выделить новый блок (за пределами уже выделенных size_buf_)
        if (block_with_offset >= size_buf_) {
            // убеждаемся, что есть место в кольцевом буфере указателей для ещё одного блока
            EnsureCapacityBlocks(size_buf_ + 1);
            // целевой индекс для нового блока
            size_t target = (start_block_ + block_with_offset) % capacity_buf_;
            if (blocks_[target] == nullptr) {
                blocks_[target] = new int[BLOCK_SIZE];
            }
            ++size_buf_;
        }

        size_t target_block = (start_block_ + block_with_offset) % capacity_buf_;
        size_t offset = (start_offset_ + size_) % BLOCK_SIZE;
        // если по каким-то причинам block_with_offset < size_buf_ но блок не выделен (неожиданный случай),
        // выделим его.
        if (blocks_[target_block] == nullptr) {
            blocks_[target_block] = new int[BLOCK_SIZE];
            ++size_buf_;
        }
        blocks_[target_block][offset] = value;
        ++size_;
    }

    // PushFront
    void PushFront(int value) {
        if (capacity_buf_ == 0) {
            EnsureCapacityBlocks(1);
        }

        if (size_ == 0) {
            // просто выделяем первый блок в текущем буфере указателей
            if (blocks_[start_block_] == nullptr) {
                blocks_[start_block_] = new int[BLOCK_SIZE];
                size_buf_ = 1;
            } else {
                size_buf_ = 1;
            }
            start_offset_ = 0;
            blocks_[start_block_][start_offset_] = value;
            ++size_;
            return;
        }

        if (start_offset_ > 0) {
            --start_offset_;
            size_t idx = start_block_;
            if (blocks_[idx] == nullptr) {
                // на всякий случай (обычно не происходит)
                blocks_[idx] = new int[BLOCK_SIZE];
                ++size_buf_;
            }
            blocks_[idx][start_offset_] = value;
            ++size_;
            return;
        }

        // start_offset_ == 0 -> нужно перейти в предыдущий блок
        // если нет места в буфере указателей, расширяем
        if (size_buf_ >= capacity_buf_) {
            EnsureCapacityBlocks(size_buf_ + 1);
        }

        size_t prev = (start_block_ + capacity_buf_ - 1) % capacity_buf_;
        if (blocks_[prev] == nullptr) {
            blocks_[prev] = new int[BLOCK_SIZE];
        }
        start_block_ = prev;
        start_offset_ = BLOCK_SIZE - 1;
        ++size_buf_;
        blocks_[start_block_][start_offset_] = value;
        ++size_;
    }

    // PopBack
    void PopBack() {
        if (size_ == 0) {
            throw std::out_of_range("Deque is empty");
        }

        if (size_ == 1) {
            // последний элемент — удаляем блок и сбрасываем состояние
            size_t last_block = start_block_;
            if (blocks_ != nullptr && capacity_buf_ > 0 && blocks_[last_block] != nullptr) {
                delete[] blocks_[last_block];
                blocks_[last_block] = nullptr;
            }
            size_ = 0;
            size_buf_ = 0;
            start_block_ = 0;
            start_offset_ = 0;
            return;
        }

        // индекс последнего элемента (относительно start)
        size_t last_total = start_offset_ + size_ - 1;
        size_t block_shift = last_total / BLOCK_SIZE;
        size_t offset = last_total % BLOCK_SIZE;
        size_t block_idx = (start_block_ + block_shift) % capacity_buf_;

        if (offset == 0) {
            // удаление элемента опустошает блок
            if (blocks_[block_idx] != nullptr) {
                delete[] blocks_[block_idx];
                blocks_[block_idx] = nullptr;
            }
            --size_;
            --size_buf_;
        } else {
            // просто уменьшаем size
            --size_;
        }
    }

    // PopFront
    void PopFront() {
        if (size_ == 0) {
            throw std::out_of_range("Deque is empty");
        }

        if (size_ == 1) {
            // удаляем последний элемент и освобождаем блок
            size_t b = start_block_;
            if (blocks_ != nullptr && capacity_buf_ > 0 && blocks_[b] != nullptr) {
                delete[] blocks_[b];
                blocks_[b] = nullptr;
            }
            size_ = 0;
            size_buf_ = 0;
            start_block_ = 0;
            start_offset_ = 0;
            return;
        }

        ++start_offset_;
        --size_;
        if (start_offset_ == BLOCK_SIZE) {
            // блок опустел — удаляем и двигаем start_block_
            size_t old = (start_block_) % capacity_buf_;
            if (blocks_[old] != nullptr) {
                delete[] blocks_[old];
                blocks_[old] = nullptr;
            }
            start_block_ = (start_block_ + 1) % capacity_buf_;
            start_offset_ = 0;
            --size_buf_;
        }
    }

    void Swap(Deque& rhs) {
        std::swap(blocks_, rhs.blocks_);
        std::swap(capacity_buf_, rhs.capacity_buf_);
        std::swap(size_buf_, rhs.size_buf_);
        std::swap(size_, rhs.size_);
        std::swap(start_block_, rhs.start_block_);
        std::swap(start_offset_, rhs.start_offset_);
    }

private:
    int** blocks_ = nullptr;     // кольцевой буфер указателей на блоки
    size_t capacity_buf_ = 0;    // вместимость буфера указателей
    size_t size_buf_ = 0;        // сколько блоков реально выделено
    size_t size_ = 0;            // количество элементов
    size_t start_block_ = 0;     // индекс блока с первым элементом (в blocks_)
    size_t start_offset_ = 0;    // смещение первого элемента в start_block_

    // Увеличить capacity буфера указателей до >= min_blocks (стратегия ×2)
    void EnsureCapacityBlocks(size_t min_blocks) {
        if (capacity_buf_ >= min_blocks) {
            return;
        }

        size_t old_capacity = capacity_buf_;
        int** old_blocks = blocks_;
        size_t old_start = start_block_;

        size_t new_capacity = std::max<size_t>(1, capacity_buf_ == 0 ? 1 : capacity_buf_ * 2);
        while (new_capacity < min_blocks) {
            new_capacity *= 2;
        }

        int** new_blocks = new int*[new_capacity];
        for (size_t i = 0; i < new_capacity; ++i) {
            new_blocks[i] = nullptr;
        }

        // копируем указатели на уже выделенные блоки в правильном порядке
        if (old_blocks != nullptr && old_capacity > 0 && size_buf_ > 0) {
            for (size_t i = 0; i < size_buf_; ++i) {
                size_t idx = (old_start + i) % old_capacity;
                new_blocks[i] = old_blocks[idx];
            }
        }

        // освобождаем старый массив указателей (не сами блоки)
        delete[] old_blocks;

        blocks_ = new_blocks;
        capacity_buf_ = new_capacity;
        start_block_ = 0;
        // size_buf_ остаётся прежним
    }
};
