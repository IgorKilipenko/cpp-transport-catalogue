#include <cassert>
#include <iostream>
#include <string>
#include <string_view>
#define _USE_MATH_DEFINES
#include <cmath>

#include "svg.h"

using namespace std::literals;
using namespace svg;

/*
Пример использования библиотеки. Он будет компилироваться и работать, когда вы реализуете
все классы библиотеки.
*/

namespace svg::tests {

    Polyline CreateStar(Point center, double outer_rad, double inner_rad, int num_rays) {
        Polyline polyline;
        for (int i = 0; i <= num_rays; ++i) {
            double angle = 2 * M_PI * (i % num_rays) / num_rays;
            polyline.AddPoint({center.x + outer_rad * sin(angle), center.y - outer_rad * cos(angle)});
            if (i == num_rays) {
                break;
            }
            angle += M_PI / num_rays;
            polyline.AddPoint({center.x + inner_rad * sin(angle), center.y - inner_rad * cos(angle)});
        }
        return polyline;
    }

    void Test1() {
        std::stringstream stream1, stream2;
        {
            stream1 << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
            stream1 << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;

            Circle c;
            c.SetCenter({20, 20}).SetRadius(10);
            RenderContext ctx(stream1, 2, 2);
            c.Render(ctx);

            stream1 << "</svg>"sv;
        }
        {
            Document doc;
            doc.Add(Circle().SetCenter({20, 20}).SetRadius(10));
            doc.Render(stream2);
        }

        const std::string result1 = stream1.str();
        const std::string result2 = stream2.str();

        assert(result1 == result2);
    }

    // Выводит приветствие, круг и звезду
    std::string DrawTestPicture() {
        std::stringstream stream;
        Document doc;
        doc.Add(Circle().SetCenter({20, 20}).SetRadius(10));
        doc.Add(
            Text().SetFontFamily("Verdana"s).SetPosition({35, 20}).SetOffset({0, 6}).SetFontSize(12).SetFontWeight("bold"s).SetData("Hello C++"s));
        doc.Add(CreateStar({20, 50}, 10, 5, 5));
        doc.Render(stream);

        return stream.str();
    }

    void Test2() {
        using namespace std::literals;
        const std::string svg_str = DrawTestPicture();
        const std::string expected_svg_str =
            R"(<?xml version="1.0" encoding="UTF-8" ?>)"
            "\n"s
            R"(<svg xmlns="http://www.w3.org/2000/svg" version="1.1">)"
            "\n"s
            R"(  <circle cx="20" cy="20" r="10" />)"
            "\n"s
            R"(  <text x="35" y="20" dx="0" dy="6" font-size="12" font-family="Verdana" font-weight="bold">Hello C++</text>)"
            "\n"s
            R"(  <polyline points="20,40 22.9389,45.9549 29.5106,46.9098 24.7553,51.5451 25.8779,58.0902 20,55 14.1221,58.0902 15.2447,51.5451 10.4894,46.9098 17.0611,45.9549 20,40" />)"
            "\n"s
            R"(</svg>)";
        assert(svg_str == expected_svg_str);
    }

}

int main() {
    svg::tests::Test1();
    std::cerr << "Test1 : Done." << std::endl;
    svg::tests::Test2();
    std::cerr << "Test2 : Done." << std::endl;
    std::cerr << std::endl << "All Tests : Done." << std::endl << std::endl;
}