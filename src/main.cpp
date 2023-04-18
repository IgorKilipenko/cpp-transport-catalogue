#include <iostream>

#include "./tests/vector_test.h"

struct Plate {
    char c1;
    int num;
    char c2;
    char c3;
    int region;
};

int main() {
    using namespace std::literals;
    std::cout << "Sizeof = "s << sizeof(Plate) << std::endl;
    std::cout << offsetof(Plate, c1) << std::endl;
    std::cout << offsetof(Plate, num) << std::endl;
    std::cout << offsetof(Plate, c2) << std::endl;
    std::cout << offsetof(Plate, c3) << std::endl;
    std::cout << offsetof(Plate, region) << std::endl;
    return 0;
}