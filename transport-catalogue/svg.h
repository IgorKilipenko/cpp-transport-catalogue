#pragma once

#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace svg::detail {
    template <typename FromType, typename ToType>
    using EnableIfConvertible = std::enable_if_t<std::is_convertible_v<std::decay_t<FromType>, ToType>, bool>;

    template <typename FromType, typename ToType>
    using IsSame = std::is_same<std::decay_t<FromType>, ToType>;

    template <typename FromType, typename ToType>
    using EnableIfSame = std::enable_if_t<std::is_same_v<std::decay_t<FromType>, ToType>, bool>;

    template <typename BaseType, typename DerivedType>
    using EnableIfBaseOf = std::enable_if_t<std::is_base_of_v<BaseType, std::decay_t<DerivedType>>, bool>;

    template <typename Out = std::string_view, typename EnumType, EnableIfConvertible<Out, std::string_view> = true>
    Out ToString(EnumType);
}

namespace svg {
    using namespace std::literals;

    using Color = std::string;

    inline const Color NoneColor{"none"};

    enum class StrokeLineCap {
        BUTT,
        ROUND,
        SQUARE,
    };

    enum class StrokeLineJoin {
        ARCS,
        BEVEL,
        MITER,
        MITER_CLIP,
        ROUND,
    };

    namespace detail {
        template <typename Out = std::string_view, EnableIfConvertible<Out, std::string_view> = true>
        Out ToString(StrokeLineCap line_cap) {
            switch (line_cap) {
            case StrokeLineCap::BUTT:
                return "butt"sv;
            case StrokeLineCap::ROUND:
                return "round"sv;
            case StrokeLineCap::SQUARE:
                return "square"sv;
            default:
                assert(false);
                break;
            }
            return ""sv;
        }

        template <typename Out = std::string_view, EnableIfConvertible<Out, std::string_view> = true>
        Out ToString(StrokeLineJoin line_join) {
            switch (line_join) {
            case svg::StrokeLineJoin::ARCS:
                return "arcs"sv;
            case svg::StrokeLineJoin::BEVEL:
                return "bevel"sv;
            case svg::StrokeLineJoin::MITER:
                return "miter"sv;
            case svg::StrokeLineJoin::MITER_CLIP:
                return "miter-clip"sv;
            case svg::StrokeLineJoin::ROUND:
                return "round"sv;
            default:
                assert(false);
                break;
            }
            return ""sv;
        }
    }

    template <
        typename EnumType,
        std::enable_if_t<detail::IsSame<EnumType, StrokeLineCap>::value || detail::IsSame<EnumType, StrokeLineJoin>::value, bool> = true>
    std::ostream& operator<<(std::ostream& os, EnumType&& val) {
        os << detail::ToString(std::move(val));
        return os;
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

    template <typename Owner>
    class PathProps {
    public:
        Owner& SetFillColor(Color color) {
            fill_color_ = std::move(color);
            return AsOwner();
        }

        Owner& SetStrokeColor(Color color) {
            stroke_color_ = std::move(color);
            return AsOwner();
        }

        Owner& SetStrokeWidth(double width) {
            stroke_width_ = width;
            return AsOwner();
        }

        Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
            stroke_linecap_ = line_cap;
            return AsOwner();
        }

        Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
            stroke_linejoin_ = line_join;
            return AsOwner();
        }

    protected:
        ~PathProps() = default;

        void RenderAttrs(std::ostream& out) const {
            using namespace std::literals;

            if (fill_color_) {
                out << " fill=\""sv << *fill_color_ << "\""sv;
            }
            if (stroke_color_) {
                out << " stroke=\""sv << *stroke_color_ << "\""sv;
            }
            if (stroke_width_) {
                out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
            }
            if (stroke_linecap_) {
                out << " stroke-linecap=\""sv << *stroke_linecap_ << "\""sv;
            }
            if (stroke_linejoin_) {
                out << " stroke-linejoin=\""sv << *stroke_linejoin_ << "\""sv;
            }
        }

    private:
        std::optional<Color> fill_color_;
        std::optional<Color> stroke_color_;
        std::optional<double> stroke_width_;
        std::optional<StrokeLineCap> stroke_linecap_;
        std::optional<StrokeLineJoin> stroke_linejoin_;

        Owner& AsOwner() {
            return static_cast<Owner&>(*this);
        }
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
    class Circle final : public Object, public PathProps<Circle> {
    public:
        Circle& SetCenter(Point center);

        Circle& SetRadius(double radius);

    private:
        void RenderObject(const RenderContext& context) const override;

        std::ostream& ToStream(std::ostream& out) const;

        Point center_;
        double radius_ = 1.0;
    };

    /*
     * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
     * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
     */
    class Polyline final : public Object, public PathProps<Polyline> {
        using Points = std::vector<Point>;

    public:
        /// Добавляет очередную вершину к ломаной линии
        template <typename Point = svg::Point, detail::EnableIfConvertible<Point, svg::Point> = true>
        Polyline& AddPoint(Point&& point);

    private:
        Points points_;

        std::ostream& PointsToStream(std::ostream& out) const;

        std::ostream& ToStream(std::ostream& out) const;

        void RenderObject(const RenderContext& context) const override;
    };

    /*
     * Класс Text моделирует элемент <text> для отображения текста
     * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
     */
    class Text final : public Object, public PathProps<Text> {
    public:
        struct TextStyle {
            Point offset;
            uint32_t size = 1;
            std::string font_family;
            std::string font_weight;
        };

        /// Задаёт координаты опорной точки (атрибуты x и y)
        template <typename Point = svg::Point, detail::EnableIfConvertible<Point, svg::Point> = true>
        Text& SetPosition(Point&& pos);

        /// Задаёт смещение относительно опорной точки (атрибуты dx, dy)
        template <typename Point = svg::Point, detail::EnableIfConvertible<Point, svg::Point> = true>
        Text& SetOffset(Point&& offset);

        /// Задаёт размеры шрифта (атрибут font-size)
        Text& SetFontSize(uint32_t size);

        /// Задаёт название шрифта (атрибут font-family)
        template <typename String = std::string, detail::EnableIfConvertible<String, std::string> = true>
        Text& SetFontFamily(String&& font_family);

        /// Задаёт толщину шрифта (атрибут font-weight)
        template <typename String = std::string, detail::EnableIfConvertible<String, std::string> = true>
        Text& SetFontWeight(String&& font_weight);

        /// Задаёт текстовое содержимое объекта (отображается внутри тега text)
        template <typename String = std::string, detail::EnableIfConvertible<String, std::string> = true>
        Text& SetData(String&& data);

    private:
        Point base_point_;
        TextStyle style_;
        std::string text_;

        std::ostream& DataToStream(std::ostream& out) const;

        std::ostream& ToStream(std::ostream& out) const;

        void RenderObject(const RenderContext& context) const override;
    };

    class ObjectContainer {
    public:
        template <typename Object = svg::Object, detail::EnableIfBaseOf<svg::Object, Object> = true>
        void Add(Object&& obj);

        virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;

    protected:
        ~ObjectContainer() = default;
    };

    class Drawable {
    public:
        virtual ~Drawable() = default;

        virtual void Draw(ObjectContainer& container) const = 0;
    };

    class Document : public ObjectContainer {
    public:
        using ObjectPtr = std::unique_ptr<Object>;
        using ObjectCollection = std::vector<ObjectPtr>;

        /// Добавляет в svg-документ объект-наследник svg::Object
        void AddPtr(ObjectPtr&& obj) override;

        /// Выводит в ostream svg-представление документа
        void Render(std::ostream& out) const;

    private:
        ObjectCollection objects_;
        static constexpr const std::string_view HEADER_LINE = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv;
        static constexpr const std::string_view SVG_TAG_OPEN = "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv;
        static constexpr const std::string_view SVG_TAG_CLOSE = "</svg>"sv;
    };

}

namespace svg /* Polyline class template impl */ {
    template <typename Point, detail::EnableIfConvertible<Point, svg::Point>>
    Polyline& Polyline::AddPoint(Point&& point) {
        points_.push_back(std::move(point));
        return *this;
    }
}

namespace svg /* Text class template impl */ {
    template <typename Point, detail::EnableIfConvertible<Point, svg::Point>>
    Text& Text::SetPosition(Point&& pos) {
        base_point_ = std::move(pos);
        return *this;
    }

    template <typename Point, detail::EnableIfConvertible<Point, svg::Point>>
    Text& Text::SetOffset(Point&& offset) {
        style_.offset = std::move(offset);
        return *this;
    }

    /// Задаёт название шрифта (атрибут font-family)
    template <typename String, detail::EnableIfConvertible<String, std::string>>
    Text& Text::SetFontFamily(String&& font_family) {
        style_.font_family = std::move(font_family);
        return *this;
    }

    /// Задаёт толщину шрифта (атрибут font-weight)
    template <typename String, detail::EnableIfConvertible<String, std::string>>
    Text& Text::SetFontWeight(String&& font_weight) {
        style_.font_weight = std::move(font_weight);
        return *this;
    }

    /// Задаёт текстовое содержимое объекта (отображается внутри тега text)
    template <typename String, detail::EnableIfConvertible<String, std::string>>
    Text& Text::SetData(String&& data) {
        text_ = std::move(data);
        return *this;
    }
}

namespace svg /* ObjectContainer class template impl */ {
    template <typename Object, detail::EnableIfBaseOf<svg::Object, Object>>
    void ObjectContainer::Add(Object&& obj) {
        AddPtr(std::make_unique<std::decay_t<Object>>(std::move(obj)));
    }
}
