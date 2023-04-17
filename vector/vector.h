#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>

template <typename T>
class Vector {
public:
    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return capacity_;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    explicit Vector(size_t n = 0) {
        data_ = Allocate(n);
        for (size_t i = 0; i != n; ++i) {
            Construct(data_ + i);
        }
        capacity_ = size_ = n;
    }

    Vector(const Vector& other) {
        data_ = Allocate(other.size_);
        for (size_t i = 0; i != other.size_; ++i) {
            Construct(data_ + i, other[i]);
        }
        capacity_ = size_ = other.size_;
    }
    ~Vector() {
        for (size_t i = 0; i != size_; ++i) {
            Destroy(data_ + i);
        }
        Deallocate(data_);
    }

    void Reserve(size_t n) {
        if (n > capacity_) {
            auto data2 = Allocate(n);
            for (size_t i = 0; i != size_; ++i) {
                Construct(data2 + i, std::move(data_[i]));
                Destroy(data_ + i);
            }
            Deallocate(data_);
            data_ = data2;
            capacity_ = n;
        }
    }

private:
    T* data_ = nullptr;
    size_t capacity_ = 0;
    size_t size_ = 0;

    static T* Allocate(size_t n) {
        return static_cast<T*>(operator new(n * sizeof(T)));
    }

    static void Deallocate(T* buf) {
        operator delete(buf);
    }

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