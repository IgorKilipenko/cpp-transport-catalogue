#include "./tests/json_test.h"
#include "./tests/svg_test.h"

int main() {
    using namespace svg::tests;
    using namespace json::tests;

    SvgTester svg_tester;
    svg_tester.TestSvglib();

    JsonTester json_tester;
    json_tester.TestJsonlib();

    return 0;
}