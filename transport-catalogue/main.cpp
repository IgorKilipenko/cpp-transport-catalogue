#include <cstdint>
#include <iostream>
#include <limits>

using namespace std;

int main() {
    int64_t a;
    int64_t b;
    cin >> a >> b;
    if ((a > 0 && b > 0 && INT64_MAX - a < b) || (a < 0 && b < 0 && INT64_MIN - a > b)) {
        cout << "Overflow!" << endl;
    } else {
        cout << a + b << endl;
    }
}