#pragma once
#include <cassert>
#include <cstdlib>
#include <memory>
#include <new>
#include <utility>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    RawMemory(size_t capacity) : buffer_(Allocate(capacity)), capacity_(capacity) {}

    ~RawMemory() {
        Deallocate(buffer_);
    }

    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    static void Deallocate(T* buffer_) noexcept {
        operator delete(buffer_);
    }

    T* operator+(size_t offset) {
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:
    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    explicit Vector(size_t n = 0) : data_(n), size_(n) {
        std::uninitialized_value_construct_n(data_.GetAddress(), n);
    }

    Vector(const Vector& other) : data_(other.size_), size_(other.size_) {
        std::uninitialized_copy_n(other.data_.GetAddress(), other.size_, data_.GetAddress());
    }
    ~Vector() {
        std::destroy_n(data_.GetAddress(), Size());
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity > data_.Capacity()) {
            RawMemory<T> new_data(new_capacity);

            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            } else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            std::destroy_n(data_.GetAddress(), Size());
            data_.Swap(new_data);
        }
    }

private:
    RawMemory<T> data_;
    size_t size_ = 0;

    static void Construct(void* buf) {
        new (buf) T();
    }

    static void Construct(void* buf, const T& elem) {
        new (buf) T(elem);
    }

    static void Construct(void* buf, const T&& elem) {
        new (buf) T(std::move(elem));
    }

    static void Destroy(T* buf) {
        buf->~T();
    }
};