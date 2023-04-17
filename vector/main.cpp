// equals_to_one_of.cpp

#include <cassert>
#include <string>
#include <string_view>

/* Напишите вашу реализацию EqualsToOneOf здесь*/

int main() {
    assert(EqualsToOneOf("hello"sv, "hi"s, "hello"s));
    assert(!EqualsToOneOf(1, 10, 2, 3, 6));
    assert(!EqualsToOneOf(8));
}