#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace svg {

    using namespace std::literals;

    namespace detail {
        template <typename FromType, typename ToType>
        using EnableIfConvertibleV = std::enable_if_t<std::is_convertible_v<std::decay_t<FromType>, ToType>, bool>;

        template <typename BaseType, typename DerivedType>
        using EnableIfBaseOfV = std::enable_if_t<std::is_base_of_v<BaseType, std::decay_t<DerivedType>>, bool>;
    }

    struct Point {
        Point() = default;
        Point(double x, double y) : x(x), y(y) {}
        double x = 0;
        double y = 0;
    };

    /*
     * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
     * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
     */
    struct RenderContext {
        RenderContext(std::ostream& out) : out(out) {}

        RenderContext(std::ostream& out, int indent_step, int indent = 0) : out(out), indent_step(indent_step), indent(indent) {}

        RenderContext Indented() const {
            return {out, indent_step, indent + indent_step};
        }

        void RenderIndent() const {
            for (int i = 0; i < indent; ++i) {
                out.put(' ');
            }
        }

        std::ostream& out;
        int indent_step = 0;
        int indent = 0;
    };

    /*
     * Абстрактный базовый класс Object служит для унифицированного хранения
     * конкретных тегов SVG-документа
     * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
     */
    class Object {
    public:
        void Render(const RenderContext& context) const;

        virtual ~Object() = default;

    private:
        virtual void RenderObject(const RenderContext& context) const = 0;
    };

    /*
     * Класс Circle моделирует элемент <circle> для отображения круга
     * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
     */
    class Circle final : public Object {
    public:
        Circle& SetCenter(Point center) {
            center_ = center;
            return *this;
        }

        Circle& SetRadius(double radius) {
            radius_ = radius;
            return *this;
        }

    private:
        void RenderObject(const RenderContext& context) const override {
            ToStream(context.out);
        }

        std::ostream& ToStream(std::ostream& out) const {
            out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
            out << "r=\""sv << radius_ << "\" "sv;
            out << "/>"sv;

            return out;
        }

        Point center_;
        double radius_ = 1.0;
    };

    /*
     * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
     * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
     */
    class Polyline final : public Object {
        using Points = std::vector<Point>;

    public:
        /// Добавляет очередную вершину к ломаной линии
        template <typename Point = svg::Point, detail::EnableIfConvertibleV<Point, svg::Point> = true>
        Polyline& AddPoint(Point&& point) {
            points_.push_back(std::move(point));
            return *this;
        }

    private:
        Points points_;

        std::ostream& PointsToStream(std::ostream& out) const {
            if (points_.empty()) {
                return out;
            }
            std::string_view sep = ""sv;
            const std::string_view comma = ","sv;

            for (const Point& point : points_) {
                out << sep << point.x << comma << point.y;
                sep = sep.empty() ? " "sv : sep;
            }

            return out;
        }

        std::ostream& ToStream(std::ostream& out) const {
            out << "<polyline points=\""sv;
            PointsToStream(out);
            out << "\" "sv;
            out << "/>"sv;
            return out;
        }

        void RenderObject(const RenderContext& context) const override {
            ToStream(context.out);
        }
    };

    /*
     * Класс Text моделирует элемент <text> для отображения текста
     * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
     */
    class Text final : public Object {
    public:
        struct TextStyle {
            Point offset;
            uint32_t size = 0;
            std::string font_family;
            std::string font_weight;
        };

        /// Задаёт координаты опорной точки (атрибуты x и y)
        template <typename Point = svg::Point, std::enable_if_t<std::is_convertible_v<std::decay_t<Point>, svg::Point>, bool> = true>
        Text& SetPosition(Point&& pos) {
            base_point_ = std::move(pos);
            return *this;
        }

        /// Задаёт смещение относительно опорной точки (атрибуты dx, dy)
        template <typename Point = svg::Point, std::enable_if_t<std::is_convertible_v<std::decay_t<Point>, svg::Point>, bool> = true>
        Text& SetOffset(Point&& offset) {
            style_.offset = std::move(offset);
            return *this;
        }

        /// Задаёт размеры шрифта (атрибут font-size)
        Text& SetFontSize(uint32_t size) {
            style_.size = size;
            return *this;
        }

        /// Задаёт название шрифта (атрибут font-family)
        template <typename String = std::string, std::enable_if_t<std::is_convertible_v<std::decay_t<String>, std::string>, bool> = true>
        Text& SetFontFamily(String&& font_family) {
            style_.font_family = move(font_family);
            return *this;
        }

        /// Задаёт толщину шрифта (атрибут font-weight)
        template <typename String = std::string, std::enable_if_t<std::is_convertible_v<std::decay_t<String>, std::string>, bool> = true>
        Text& SetFontWeight(String&& font_weight) {
            style_.font_weight = font_weight;
            return *this;
        }

        /// Задаёт текстовое содержимое объекта (отображается внутри тега text)
        template <typename String = std::string, std::enable_if_t<std::is_convertible_v<std::decay_t<String>, std::string>, bool> = true>
        Text& SetData(String&& data) {
            text_.clear();
            DataPreprocessing(data, text_);
            return *this;
        }

    private:
        Point base_point_;
        TextStyle style_;
        std::string text_;

        void DataPreprocessing(const std::string_view data, std::string& result) const {
            for (char c : data) {
                switch (c) {
                case '"':
                    result.append("&quot;"s);
                    break;
                case '\'':
                    result.append("&apos;"s);
                    break;
                case '<':
                    result.append("&lt;"s);
                    break;
                case '>':
                    result.append("&gt;"s);
                    break;
                case '&':
                    result.append("&amp;"s);
                    break;
                default:
                    result.push_back(c);
                    break;
                }
            }
        }

        std::ostream& ToStream(std::ostream& out) const {
            out << "<text x=\""sv << base_point_.x << "\" y=\""sv << base_point_.y << "\""sv;
            out << " dx=\""sv << style_.offset.x << "\" dy=\""sv << style_.offset.y << "\""sv;
            out << " font-size=\""sv << style_.size << "\"";

            if (!style_.font_family.empty()) {
                out << " font-family=\""sv << style_.font_family << "\""sv;
            }
            if (!style_.font_weight.empty()) {
                out << " font-weight=\""sv << style_.font_weight << "\""sv;
            }

            out << ">"sv << text_ << "</text>"sv;

            return out;
        }

        void RenderObject(const RenderContext& context) const override {
            ToStream(context.out);
        }
    };

    class Document {
    public:
        using ObjectPtr = std::unique_ptr<Object>;
        using ObjectCollection = std::vector<ObjectPtr>;
        /*
         Метод Add добавляет в svg-документ любой объект-наследник svg::Object.
         Пример использования:
         Document doc;
         doc.Add(Circle().SetCenter({20, 30}).SetRadius(15));
        */

        template <typename Object, detail::EnableIfBaseOfV<svg::Object, Object> = true>
        void Add(Object&& obj) {
            objects_.emplace_back(std::make_unique<std::decay_t<Object>>(std::move(obj)));
        }

        /// Добавляет в svg-документ объект-наследник svg::Object
        void AddPtr(ObjectPtr&& obj) {
            objects_.emplace_back(std::move(obj));
        }

        /// Выводит в ostream svg-представление документа
        void Render(std::ostream& out) const {
            out << HEADER_LINE << "\n"sv << SVG_TAG_OPEN << "\n"sv;

            for (const auto& obj : objects_) {
                RenderContext ctx(out, 2, 2);
                obj->Render(ctx);
            }

            out << SVG_TAG_CLOSE;
            out.flush();
        }

    private:
        ObjectCollection objects_;
        static constexpr const std::string_view HEADER_LINE = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv;
        static constexpr const std::string_view SVG_TAG_OPEN = "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv;
        static constexpr const std::string_view SVG_TAG_CLOSE = "</svg>"sv;
    };

}  // namespace svg