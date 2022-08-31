#include <cassert>
#include <functional>
#include <mutex>
#include <optional>
#include <string>

using namespace std;

template <typename T>
class LazyValue {
    mutable std::mutex mtx;
    mutable std::optional<T> value_;
    std::function<T()> init_;

public:
    explicit LazyValue(function<T()> init) : init_(init){};

    bool HasValue() const {
        return value_.has_value();
    };
    const T& Get() const {
        {
            std::lock_guard guard(mtx);
            if (!value_.has_value()) {
                value_ = init_();
            }
        }
        return *value_;
    };
};

inline void UseExample() {
    const string big_string = "Giant amounts of memory"s;

    LazyValue<string> lazy_string([&big_string] {
        return big_string;
    });

    assert(!lazy_string.HasValue());
    assert(lazy_string.Get() == big_string);
    assert(lazy_string.Get() == big_string);
}

inline void TestInitializerIsntCalled() {
    bool called = false;

    {
        LazyValue<int> lazy_int([&called] {
            called = true;
            return 0;
        });
    }
    assert(!called);
}

/*
int main() {
    UseExample();
    TestInitializerIsntCalled();
}
*/