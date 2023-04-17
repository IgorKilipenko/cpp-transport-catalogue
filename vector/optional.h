#include <stdexcept>
#include <utility>

// Исключение этого типа должно генерироватся при обращении к пустому optional
class BadOptionalAccess : public std::exception {
public:
    using exception::exception;

    virtual const char* what() const noexcept override {
        return "Bad optional access";
    }
};

template <typename T>
class Optional {
public:
    Optional() = default;
    
    Optional(const T& value) {
        if (is_initialized_) {
            *ptr_ = value;
        } else {
            ptr_ = new (&data_[0]) T(value);
        }
        is_initialized_ = true;
    }

    Optional(T&& value) {
        if (is_initialized_) {
            *ptr_ = std::move(value);
        } else {
            ptr_ = new (&data_[0]) T(std::move(value));
        }
        is_initialized_ = true;
    }

    Optional(const Optional& other) {
        if (is_initialized_ && other.is_initialized_) {
            *ptr_ = *other.ptr_;
        } else if (other.is_initialized_) {
            ptr_ = new (&data_[0]) T(*other.ptr_);
        }
        is_initialized_ = other.is_initialized_;
    }

    Optional(Optional&& other) {
        if (is_initialized_ && other.is_initialized_) {
            *ptr_ = std::move(*other.ptr_);
        } else if (other.is_initialized_) {
            ptr_ = new (&data_[0]) T(std::move(*other.ptr_));
        }
        is_initialized_ = other.is_initialized_;
    }

    Optional& operator=(const T& value) {
        if (is_initialized_) {
            *ptr_ = value;
        } else {
            ptr_ = new (&data_[0]) T(value);
        }
        is_initialized_ = true;
        return *this;
    }

    Optional& operator=(T&& rhs) {
        if (is_initialized_) {
            *ptr_ = std::move(rhs);
        } else {
            ptr_ = new (&data_[0]) T(std::move(rhs));
        }
        is_initialized_ = true;
        return *this;
    }

    Optional& operator=(const Optional& rhs) {
        if (is_initialized_ && rhs.is_initialized_) {
            *ptr_ = *rhs.ptr_;
        } else if (rhs.is_initialized_) {
            ptr_ = new (&data_[0]) T(*rhs.ptr_);
        } else {
            Reset();
        }
        is_initialized_ = rhs.is_initialized_;
        return *this;
    }

    Optional& operator=(Optional&& rhs) {
        if (is_initialized_ && rhs.is_initialized_) {
            *ptr_ = std::move(*rhs.ptr_);
        } else if (rhs.is_initialized_) {
            ptr_ = new (&data_[0]) T(std::move(*rhs.ptr_));
        } else {
            Reset();
        }
        is_initialized_ = rhs.is_initialized_;
        return *this;
    }

    ~Optional() {
        Reset();
    }

    bool HasValue() const {
        return is_initialized_;
    }

    // Операторы * и -> не должны делать никаких проверок на пустоту Optional.
    // Эти проверки остаются на совести программиста
    T& operator*() & {
        return *ptr_;
    }
    const T& operator*() const& {
        return *ptr_;
    }
    T* operator->() {
        return ptr_;
    }
    const T* operator->() const {
        return ptr_;
    }

    T&& operator*() && {
        return std::move(*ptr_);
    }

    T&& Value() && {
        if (!is_initialized_) {
            throw BadOptionalAccess();
        }
        return std::move(*ptr_);
    }

    // Метод Value() генерирует исключение BadOptionalAccess, если Optional пуст
    T& Value() & {
        if (!is_initialized_) {
            throw BadOptionalAccess();
        }
        return *ptr_;
    }
    
    const T& Value() const& {
        if (!is_initialized_) {
            throw BadOptionalAccess();
        }
        return *ptr_;
    }

    template <typename... Types>
    void Emplace(Types&&... values) {
        Reset();
        ptr_ = new (&data_[0]) T(std::forward<Types>(values)...);
        is_initialized_ = true;
    }

    void Reset() {
        if (is_initialized_) {
            ptr_->~T();
            is_initialized_ = false;
        }
    }

private:
    // alignas нужен для правильного выравнивания блока памяти
    alignas(T) char data_[sizeof(T)];
    bool is_initialized_ = false;
    T* ptr_ = nullptr;
};