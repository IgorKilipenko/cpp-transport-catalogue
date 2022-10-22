#include <iostream>
#include <string_view>
#include <unordered_set>
#include <string>

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