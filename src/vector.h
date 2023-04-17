#pragma once
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <new>
#include <utility>

namespace /* template type helpers */ {
    template <typename FromType, typename ToType>
    using EnableIfSame = std::enable_if_t<std::is_same_v<std::decay_t<FromType>, ToType>, bool>;
}

namespace /* RawMemory */ {
    template <typename T>
    class RawMemory {
    public:
        RawMemory() = default;

        RawMemory(const RawMemory&) = delete;

        RawMemory& operator=(const RawMemory&) = delete;

        RawMemory& operator=(RawMemory&& other) noexcept {
            Swap(other);
            return *this;
        }

        RawMemory(RawMemory&& other) noexcept {
            Swap(other);
        }

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
            assert(index < capacity_);
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
}

namespace /* Vector */ {
    template <typename T>
    class Vector {
    public:
        using iterator = T*;
        using const_iterator = const T*;

        Vector() = default;

        explicit Vector(size_t n) : data_(n) {
            std::uninitialized_value_construct_n(data_.GetAddress(), n);
            size_ = n;
        }

        Vector(Vector&& other) noexcept {
            Swap(other);
        }

        Vector(const Vector& other) : data_(other.size_) {
            std::uninitialized_copy_n(other.data_.GetAddress(), other.size_, data_.GetAddress());
            size_ = other.size_;
        }

        ~Vector() {
            std::destroy_n(data_.GetAddress(), Size());
        }

        iterator begin() noexcept;
        iterator end() noexcept;
        const_iterator begin() const noexcept;
        const_iterator end() const noexcept;
        const_iterator cbegin() const noexcept;
        const_iterator cend() const noexcept;

        const T& operator[](size_t index) const noexcept;
        T& operator[](size_t index) noexcept;
        Vector& operator=(const Vector& other);
        Vector& operator=(Vector&& other) noexcept;

        iterator Erase(const_iterator pos);
        template <typename TItem, EnableIfSame<TItem, T> = true>
        iterator Insert(const_iterator pos, TItem&& value);
        template <typename... Args>
        iterator Emplace(const_iterator pos, Args&&... args);
        template <typename... Args>
        T& EmplaceBack(Args&&... args);

        size_t Size() const noexcept;
        size_t Capacity() const noexcept;
        void Swap(Vector& other) noexcept;
        void Reserve(size_t new_capacity);
        void Resize(size_t new_size);

        template <typename TItem, EnableIfSame<TItem, T> = true>
        void PushBack(TItem&& value);
        void PopBack();

    private:
        RawMemory<T> data_;
        size_t size_ = 0;

    private:
        [[nodiscard]] RawMemory<T> CopyData_(size_t count) {
            RawMemory<T> new_data(count);

            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            } else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }

            return new_data;
        }
    };
}

namespace /* Vector impl */ {
    template <typename T>
    typename Vector<T>::iterator Vector<T>::begin() noexcept {
        return const_cast<iterator>(cbegin());
    }

    template <typename T>
    typename Vector<T>::const_iterator Vector<T>::begin() const noexcept {
        return cbegin();
    }

    template <typename T>
    typename Vector<T>::const_iterator Vector<T>::cbegin() const noexcept {
        return data_.GetAddress();
    }

    template <typename T>
    typename Vector<T>::iterator Vector<T>::end() noexcept {
        return const_cast<iterator>(cend());
    }

    template <typename T>
    typename Vector<T>::const_iterator Vector<T>::end() const noexcept {
        return cend();
    }

    template <typename T>
    typename Vector<T>::const_iterator Vector<T>::cend() const noexcept {
        return (data_.GetAddress() + size_);
    }

    template <typename T>
    template <typename TItem, EnableIfSame<TItem, T>>
    typename Vector<T>::iterator Vector<T>::Insert(const_iterator pos, TItem&& value) {
        return Emplace(pos, std::forward<TItem>(value));
    }

    template <typename T>
    template <typename... Args>
    typename Vector<T>::iterator Vector<T>::Emplace(const_iterator pos, Args&&... args) {
        const auto insert = [this](size_t index, T&& item) {
            new (data_ + size_) T(std::move(data_[size_ - 1u]));
            std::move_backward(data_.GetAddress() + index, end() - 1, end());
            data_[index] = std::move(item);
            size_++;
        };

        size_t index = std::distance(cbegin(), pos);
        if (size_ == index) {
            EmplaceBack(std::forward<Args>(args)...);
        } else if (size_ < Capacity()) {
            insert(index, T(std::forward<Args>(args)...));
        } else {
            RawMemory<T> new_data(size_ * 2);
            new (new_data + index) T(std::forward<Args>(args)...);

            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), index, new_data.GetAddress());
                std::uninitialized_move_n(data_.GetAddress() + index, size_ - index, new_data.GetAddress() + index + 1);
            } else {
                std::uninitialized_copy_n(data_.GetAddress(), index, new_data.GetAddress());
                std::uninitialized_copy_n(data_.GetAddress() + index, size_ - index, new_data.GetAddress() + index + 1);
            }

            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
            size_++;
        }

        return (data_.GetAddress() + index);
    }

    template <typename T>
    typename Vector<T>::iterator Vector<T>::Erase(const_iterator pos) {
        iterator pos_it = const_cast<iterator>(pos);
        std::move(pos_it + 1, end(), pos_it);
        std::destroy_n(data_.GetAddress() + (--size_), 1);
        return pos_it;
    }

    template <typename T>
    size_t Vector<T>::Size() const noexcept {
        return size_;
    }

    template <typename T>
    size_t Vector<T>::Capacity() const noexcept {
        return data_.Capacity();
    }

    template <typename T>
    void Vector<T>::Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

    template <typename T>
    const T& Vector<T>::operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    template <typename T>
    T& Vector<T>::operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    template <typename T>
    Vector<T>& Vector<T>::operator=(const Vector& other) {
        if (other.Size() > data_.Capacity()) {
            Vector tmp(other);
            Swap(tmp);
            return *this;
        }

        else {
            for (size_t i = 0; i < Size() && i < other.Size(); ++i) {
                data_[i] = other[i];
            }
            if (Size() < other.Size()) {
                std::uninitialized_copy_n(other.data_.GetAddress() + Size(), other.Size() - Size(), data_.GetAddress());
            } else if (Size() > other.Size()) {
                std::destroy_n(data_.GetAddress() + other.Size(), Size() - other.Size());
            }
            size_ = other.Size();
        }
        return *this;
    }

    template <typename T>
    Vector<T>& Vector<T>::operator=(Vector&& other) noexcept {
        Swap(other);
        return *this;
    }

    template <typename T>
    void Vector<T>::Reserve(size_t new_capacity) {
        if (new_capacity > data_.Capacity()) {
            RawMemory<T> new_data = CopyData_(new_capacity);

            std::destroy_n(data_.GetAddress(), Size());
            data_.Swap(new_data);
        }
    }

    template <typename T>
    void Vector<T>::Resize(size_t new_size) {
        Reserve(new_size);
        if (size_ < new_size) {
            std::uninitialized_value_construct_n(data_.GetAddress() + Size(), new_size - Size());

        } else if (size_ > new_size) {
            std::destroy_n(data_.GetAddress() + new_size, Size() - new_size);
        }
        size_ = new_size;
    }

    template <typename T>
    template <typename TItem, EnableIfSame<TItem, T>>
    void Vector<T>::PushBack(TItem&& value) {
        if (size_ == Capacity()) {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            new (new_data + size_) T(std::forward<TItem>(value));
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            } else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        } else {
            new (data_ + size_) T(std::forward<TItem>(value));
        }
        ++size_;
    }

    template <typename T>
    void Vector<T>::PopBack() {
        std::destroy_at(data_.GetAddress() + size_ - 1);
        --size_;
    }

    template <typename T>
    template <typename... Args>
    T& Vector<T>::EmplaceBack(Args&&... args) {
        if (size_ == Capacity()) {
            size_t capacity_tmp = 0;
            size_ == 0 ? capacity_tmp += 1 : capacity_tmp += size_ * 2;

            RawMemory<T> new_data(capacity_tmp);
            new (new_data + size_) T(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            } else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }

            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        } else {
            new (data_ + size_) T(std::forward<Args>(args)...);
        }
        size_++;
        return data_[size_ - 1];
    }
}
