#include <iostream>
#include <set>
#include <string_view>

using namespace std;

// clang-format off

const std::set<std::string_view> PLANETS = {
    "Mercury"sv, "Venus"sv, "Earth"sv,
    "Mars"sv, "Jupiter"sv, "Saturn"sv,
    "Uranus"sv, "Neptune"sv
};

// clang-format on

bool IsPlanet(string_view name) {
    return PLANETS.count(name);
}

void Test(string_view name) {
    cout << name << " is " << (IsPlanet(name) ? ""sv : "NOT "sv) << "a planet"sv << endl;
}

int main() {
    std::string str;
    std::getline(std::cin, str);

    Test(str);
}

/*
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_set>

using namespace std;


// clang-format off

const unordered_set<string_view> PLANETS {
        "Mercury"sv, "Venus"sv, "Earth"sv,
        "Mars"sv, "Jupiter"sv, "Saturn"sv,
        "Uranus"sv, "Neptune"sv
};

// clang-format on

bool IsPlanet(string_view name) {
    if (PLANETS.count(name)) {
        return true;
    }
    return false;
}

void Test(string_view name) {
    cout << name << " is " << (IsPlanet(name) ? ""sv : "NOT "sv)
         << "a planet"sv << endl;
}

int main() {
    string planet;
    getline(cin, planet);
    Test(planet);
}
*/