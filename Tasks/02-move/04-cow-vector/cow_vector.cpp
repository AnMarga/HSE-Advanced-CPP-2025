#include "cow_vector.h"
#include <cstddef>
#include <utility>

// Your code goes here
COWVector::COWVector() {
    state_ = new State();
}

COWVector::~COWVector() {
    if (--state_->ref_count == 0) {
        delete state_;
    }
}

COWVector::COWVector(const COWVector& other) {
    state_ = other.state_;
    state_->ref_count += 1;
}

COWVector& COWVector::operator=(const COWVector& other) {
    if (this != &other) {
        if (--state_->ref_count == 0) {
            delete state_;
        }
        state_ = other.state_;
        state_->ref_count += 1;
    }
    return *this;
}

COWVector::COWVector(COWVector&& other) noexcept {
    state_ = std::exchange(other.state_, nullptr);
}

COWVector& COWVector::operator=(COWVector&& other) noexcept {
    if (state_) {
        delete state_;
    }
    state_ = std::exchange(other.state_, nullptr);
    return *this;
}

size_t COWVector::Size() const {
    return this->state_->vec.size();
}

void COWVector::Resize(size_t size) {
    Unlink();
    this->state_->vec.resize(size);
}

const std::string& COWVector::Get(size_t at) {
    return this->state_->vec.at(at);
}

const std::string& COWVector::Back() {
    return this->state_->vec.back();
}

void COWVector::PushBack(const std::string& value) {
    Unlink();
    this->state_->vec.push_back(value);
}

void COWVector::Set(size_t at, const std::string& value) {
    Unlink();
    this->state_->vec[at] = value;
}

void COWVector::Unlink() {
    if (state_->ref_count > 1) {
        state_->ref_count--;
        state_ = new State(*state_);
        state_->ref_count = 1;
    }
}
