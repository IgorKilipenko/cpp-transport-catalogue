// equals_to_one_of.cpp

#include <cassert>
#include <string>
#include <string_view>

/* Напишите вашу реализацию EqualsToOneOf здесь*/
template <typename... T>
bool EqualsToOneOf(const T&... values) {
    if (sizeof...(values) == 1) {
        return false;
    }
}

template <typename T1, typename T2, typename... Ts>
bool EqualsToOneOf(const T1& first, const T2& second, const Ts&... args) {
    return (first == second) || EqualsToOneOf(first, args...);
}

int main() {
    using namespace std::literals;
    assert(EqualsToOneOf("hello"sv, "hi"s, "hello"s));
    assert(!EqualsToOneOf(1, 10, 2, 3, 6));
    assert(!EqualsToOneOf(8));
}