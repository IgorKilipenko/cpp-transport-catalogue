#include <iostream>
#include <limits>

int main() {
    int64_t a, b;
    std::cin >> a >> b;
    const int64_t min = -10;// std::numeric_limits<int64_t>::min();
    const int64_t max = 10;//std::numeric_limits<int64_t>::max();

    if ((a > 0 && b > 0 && max - a < b) || 
        (a < 0 && b < 0 && min - a > b)) {
        std::cout << "Overflow!" << std::endl;
    } else {
        std::cout << a + b << std::endl;
    }
}