#include <cassert>
#include <cstddef>  // нужно для nullptr_t
#include <tuple>

using namespace std;

// Реализуйте шаблон класса UniquePtr
template <typename T>
class UniquePtr {
private:
    T* ptr_ = nullptr;

public:
    UniquePtr() = default;
    explicit UniquePtr(T* ptr) : ptr_{std::move(ptr)} {}

    UniquePtr(const UniquePtr&) = delete;

    UniquePtr& operator=(const UniquePtr&) = delete;

    UniquePtr& operator=(nullptr_t) {
        UniquePtr<T> temp(nullptr);
        Swap(temp);
        return *this;
    }

    UniquePtr(UniquePtr&& other) {
        if (this == &other) {
            return;
        }
        Swap(other);
    }

    UniquePtr& operator=(UniquePtr&& other) {
        if (this == &other) {
            return *this;
        }
        Swap(other);
        return *this;
    }

    ~UniquePtr() {
        delete (ptr_);
    }

    T& operator*() const {
        return *ptr_;
    }
    T* operator->() const {
        return ptr_;
    }
    T* Release() {
        T* tmp = std::move(ptr_);
        ptr_ = nullptr;
        return tmp;
    }
    void Reset(T* ptr) {
        delete ptr_;
        ptr_ = ptr;
    }
    void Swap(UniquePtr& other) {
        std::swap(ptr_, other.ptr_);
    }
    T* Get() const {
        return ptr_;
    }
};

struct Item {
    static int counter;
    int value;
    Item(int v = 0) : value(v) {
        ++counter;
    }
    Item(const Item& other) : value(other.value) {
        ++counter;
    }
    ~Item() {
        --counter;
    }
};

int Item::counter = 0;

void TestLifetime() {
    Item::counter = 0;
    {
        UniquePtr<Item> ptr(new Item);
        assert(Item::counter == 1);

        ptr.Reset(new Item);
        assert(Item::counter == 1);
    }
    assert(Item::counter == 0);

    {
        UniquePtr<Item> ptr(new Item);
        assert(Item::counter == 1);

        auto rawPtr = ptr.Release();
        assert(Item::counter == 1);

        delete rawPtr;
        assert(Item::counter == 0);
    }
    assert(Item::counter == 0);
}

void TestGetters() {
    UniquePtr<Item> ptr(new Item(42));
    assert(ptr.Get()->value == 42);
    assert((*ptr).value == 42);
    assert(ptr->value == 42);
}

int main() {
    TestLifetime();
    TestGetters();
    return 0;
}