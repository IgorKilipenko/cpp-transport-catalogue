#include <person.pb.h>

#include <iostream>

int main() {
    person::Address addr;
    addr.set_building(15);
    std::cout << addr.building() << std::endl;
}