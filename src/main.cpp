#include <iostream>
#include <istream>
#include <sstream>

#include "./tests/vector_test.h"

struct Inner {
    int i;
    char c;
};

struct Outer0 {
    Inner in1;
    char c;
    Inner in2;
};

struct X {
    std::array<int, 3> i;
    double d;
    char c;
};
#pragma pack(push, 1)
struct Plate {
    uint32_t num:10;
    uint32_t region:10;
    uint32_t c1:4;
    uint32_t c2:4;
    uint32_t c3:4;
};
#pragma pack(pop)

struct Base1 {
    int i;
    char c;
};

struct Base2 : Base1 {
    bool b;
    float f;
};

struct Outer : Base2 {
    short sh;
};

template <typename... Args>
void print(Args... args) {
    std::stringstream ss;
    ((ss << sizeof(args) << ", "), ...);
    std::string str(ss.str());
    std::cout << (str.ends_with(", ") ? str.replace(str.end()-2, str.end(), 1, '\0') : str) << std::endl;
}

int main() {
    using namespace std::literals;
    // std::cout << sizeof(Plate) << " " << offsetof(Plate, c1) << " " << offsetof(Plate, c1) << std::endl;
    const Plate p{};
    print(p, p.c1, p.c2, p.c3, p.num, p.region);
    return 0;
}