#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#define _USE_MATH_DEFINES
#include <cmath>

#include "svg.h"

using namespace std::literals;
using namespace svg;

namespace shapes {
    class Triangle : public svg::Drawable {
    public:
        Triangle(svg::Point p1, svg::Point p2, svg::Point p3) : p1_(p1), p2_(p2), p3_(p3) {}

        // Реализует метод Draw интерфейса svg::Drawable
        void Draw(svg::ObjectContainer& container) const override {
            container.Add(svg::Polyline().AddPoint(p1_).AddPoint(p2_).AddPoint(p3_).AddPoint(p1_));
        }

    private:
        svg::Point p1_, p2_, p3_;
    };

    class Star : public Drawable { /* Реализуйте самостоятельно */
    public:
        Star(svg::Point center, double outer_radius, double inner_radius, int num_rays)
            : center_(center), outer_radius_(outer_radius), inner_radius_(inner_radius), num_rays_(num_rays) {}

        void Draw(ObjectContainer& container) const override {
            Polyline polyline;
            for (int i = 0; i <= num_rays_; ++i) {
                double angle = 2 * M_PI * (i % num_rays_) / num_rays_;
                polyline.AddPoint({center_.x + outer_radius_ * sin(angle), center_.y - outer_radius_ * cos(angle)});
                if (i == num_rays_) {
                    break;
                }
                angle += M_PI / num_rays_;
                polyline.AddPoint({center_.x + inner_radius_ * sin(angle), center_.y - inner_radius_ * cos(angle)});
            }
            container.Add(polyline);
        }

    private:
        svg::Point center_;
        double outer_radius_;
        double inner_radius_;
        int num_rays_;
    };
    class Snowman : public Drawable { /* Реализуйте самостоятельно */
    public:
        Snowman(svg::Point head_center, double head_radius) : head_center_(head_center), head_radius(head_radius) {}

        void Draw(ObjectContainer& container) const override {
            container.Add(Circle().SetCenter({head_center_.x, head_center_.y + head_radius * 5}).SetRadius(2 * head_radius));
            container.Add(Circle().SetCenter({head_center_.x, head_center_.y + head_radius * 2}).SetRadius(1.5 * head_radius));
            container.Add(Circle().SetCenter(head_center_).SetRadius(head_radius));
        }

    private:
        svg::Point head_center_;
        double head_radius;
    };

    svg::Polyline CreateStar(svg::Point center, double outer_rad, double inner_rad, int num_rays) {
        using namespace svg;
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

    template <typename DrawableIterator>
    void DrawPicture(DrawableIterator begin, DrawableIterator end, svg::ObjectContainer& target) {
        for (auto it = begin; it != end; ++it) {
            (*it)->Draw(target);
        }
    }

    template <typename Container>
    void DrawPicture(const Container& container, svg::ObjectContainer& target) {
        using namespace std;
        DrawPicture(begin(container), end(container), target);
    }
}

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

    namespace shapes {

        void Test1() {
            const std::string expected_svg_str =
                R"(<?xml version="1.0" encoding="UTF-8" ?>)"
                "\n"s
                R"(<svg xmlns="http://www.w3.org/2000/svg" version="1.1">)"
                "\n"s
                R"(  <polyline points="100,20 120,50 80,40 100,20" />)"
                "\n"s
                R"(  <polyline points="50,10 52.3511,16.7639 59.5106,16.9098 53.8042,21.2361 55.8779,28.0902 50,24 44.1221,28.0902 46.1958,21.2361 40.4894,16.9098 47.6489,16.7639 50,10" />)"
                "\n"s
                R"(  <circle cx="30" cy="70" r="20" />)"
                "\n"s
                R"(  <circle cx="30" cy="40" r="15" />)"
                "\n"s
                R"(  <circle cx="30" cy="20" r="10" />)"
                "\n"s
                R"(</svg>)";

            std::stringstream stream;
            std::vector<std::unique_ptr<svg::Drawable>> picture;

            picture.emplace_back(std::make_unique<::shapes::Triangle>(Point{100, 20}, Point{120, 50}, Point{80, 40}));
            // 5-лучевая звезда с центром {50, 20}, длиной лучей 10 и внутренним радиусом 4
            picture.emplace_back(std::make_unique<::shapes::Star>(Point{50.0, 20.0}, 10.0, 4.0, 5));
            // Снеговик с "головой" радиусом 10, имеющей центр в точке {30, 20}
            picture.emplace_back(std::make_unique<::shapes::Snowman>(Point{30, 20}, 10.0));

            svg::Document doc;
            // Так как документ реализует интерфейс ObjectContainer,
            // его можно передать в DrawPicture в качестве цели для рисования
            ::shapes::DrawPicture(picture, doc);

            // Выводим полученный документ в stdout
            doc.Render(stream);

            std::string result = stream.str();
            std::cerr << result << std::endl;

            assert(result == expected_svg_str);
        }
    }
}

int main() {
    // using namespace svg;
    // using namespace svg::tests::shapes;
    //  using namespace std;

    svg::tests::Test1();
    std::cerr << "Test1 : Done." << std::endl;

    svg::tests::Test2();
    std::cerr << "Test2 : Done." << std::endl;

    svg::tests::shapes::Test1();
    std::cerr << "shapes::Test1 : Done." << std::endl;

    std::cerr << std::endl << "All Tests : Done." << std::endl << std::endl;
}