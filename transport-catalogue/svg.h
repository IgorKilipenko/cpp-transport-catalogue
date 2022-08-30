#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace svg::detail {
    template <typename FromType, typename ToType>
    using EnableIfConvertible = std::enable_if_t<std::is_convertible_v<std::decay_t<FromType>, ToType>, bool>;

    template <typename FromType, typename ToType>
    using IsConvertible = std::is_convertible<std::decay_t<FromType>, ToType>;

    template <typename FromType, typename ToType>
    using IsSame = std::is_same<std::decay_t<FromType>, ToType>;

    template <typename FromType, typename ToType>
    using EnableIfSame = std::enable_if_t<std::is_same_v<std::decay_t<FromType>, ToType>, bool>;

    template <typename BaseType, typename DerivedType>
    using EnableIfBaseOf = std::enable_if_t<std::is_base_of_v<BaseType, std::decay_t<DerivedType>>, bool>;
}

namespace svg /* Stroke */ {

    class Stroke {
    public:
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

        template <
            typename Out = std::string_view, typename EnumType,
            std::enable_if_t<
                detail::IsConvertible<Out, std::string_view>::value &&
                    (detail::IsSame<EnumType, StrokeLineCap>::value || detail::IsSame<EnumType, StrokeLineJoin>::value),
                bool> = true>
        static Out ToString(EnumType line_cap);
    };
}

namespace svg /* Colors */ {
    class Colors {
    public:
        struct Rgb;
        struct Rgba;
        using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;

        struct Rgb {
            uint8_t red = 0;
            uint8_t green = 0;
            uint8_t blue = 0;

            Rgb(uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0) : red(red), green(green), blue(blue) {}

            static Rgb FromVariant(std::vector<std::variant<uint8_t, double>>&& value) {
                assert(value.size() >= 3);
                return {std::get<uint8_t>(value[0]), std::get<uint8_t>(value[1]), std::get<uint8_t>(value[2])};
            }

            /// Выполняет линейную интерполяцию Rgb цвета от from до to в зависимости от параметра t
            template <typename Rgb, detail::EnableIfConvertible<Rgb, Colors::Rgb> = true>
            Rgb Lerp(Rgb&& to, double t) {
                return Colors::Lerp(*this, to, t);
            }
        };

        struct Rgba : Rgb {
            double opacity = 1.;

            Rgba(uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0, double opacity = 1.) : Rgb(red, green, blue), opacity{opacity} {}

            static Rgba FromVariant(std::vector<std::variant<uint8_t, double>>&& value) {
                assert(value.size() >= 4);
                return {std::get<uint8_t>(value[0]), std::get<uint8_t>(value[1]), std::get<uint8_t>(value[2]), std::get<double>(value[3])};
            }
        };

        class ColorPrinter {
        public:
            explicit ColorPrinter(std::ostream& out) : out_(out) {}

            void operator()(std::monostate) const;

            template <typename String, detail::EnableIfConvertible<String, std::string_view> = true>
            void operator()(String&& color) const;

            template <typename RgbType = Colors::Rgb, detail::EnableIfSame<RgbType, Colors::Rgb> = true>
            void operator()(RgbType&& color) const;

            template <typename RgbaType = Colors::Rgba, detail::EnableIfSame<RgbaType, Colors::Rgba> = true>
            void operator()(RgbaType&& color) const;

            std::ostream& GetStream() const;

        private:
            std::ostream& out_;
            static constexpr const std::string_view SEPARATOR{","};

            template <typename RgbType = Colors::Rgb, detail::EnableIfSame<RgbType, Colors::Rgb> = true>
            std::ostream& Print(RgbType&& color) const;

            template <typename RgbaType = Colors::Rgba, detail::EnableIfSame<RgbaType, Colors::Rgba> = true>
            std::ostream& Print(RgbaType&& color) const;
        };

    private:
        static constexpr const std::string_view NONE{"none"};

    public:
        static inline const Color NoneColor{static_cast<std::string>(Colors::NONE)};

        /// Выполняет линейную интерполяцию значения от from до to в зависимости от параметра t
        static uint8_t Lerp(uint8_t from, uint8_t to, double t);

        // Выполняет линейную интерполяцию Rgb цвета от from до to в зависимости от параметра t
        template <
            typename RgbFrom, typename RgbTo,
            std::enable_if_t<detail::IsConvertible<RgbFrom, Rgb>::value && detail::IsConvertible<RgbTo, Rgb>::value, bool> = true>
        static Rgb Lerp(RgbFrom&& from, RgbTo to, double t);
    };
}

namespace svg {
    using namespace std::literals;

    using Color = svg::Colors::Color;
    using Rgb = svg::Colors::Rgb;
    using Rgba = svg::Colors::Rgba;
    static inline const Color NoneColor = svg::Colors::NoneColor;

    using StrokeLineCap = svg::Stroke::StrokeLineCap;
    using StrokeLineJoin = svg::Stroke::StrokeLineJoin;

    template <
        typename EnumType,
        std::enable_if_t<detail::IsSame<EnumType, StrokeLineCap>::value || detail::IsSame<EnumType, StrokeLineJoin>::value, bool> = true>
    std::ostream& operator<<(std::ostream& os, EnumType&& val) {
        os << Stroke::ToString(std::forward<EnumType>(val));
        return os;
    }

    template <typename Color, detail::EnableIfSame<Color, svg::Color> = true>
    std::ostream& operator<<(std::ostream& os, Color&& color) {
        std::visit(svg::Colors::ColorPrinter{os}, std::forward<Color>(color));
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
}

namespace svg /* PathProps */ {

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

            const Colors::ColorPrinter color_printer{out};

            if (fill_color_) {
                out << " fill=\""sv;
                std::visit(color_printer, std::move(*fill_color_));
                out << "\""sv;
            }
            if (stroke_color_) {
                out << " stroke=\""sv;
                std::visit(std::move(color_printer), std::move(*stroke_color_));
                out << "\""sv;
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
}

namespace svg /* Svg Objects */ {

    /*
     * Абстрактный базовый класс Object служит для унифицированного хранения
     * конкретных тегов SVG-документа
     * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
     */
    class Object {
    public:
        void Render(const RenderContext& context) const;

        virtual std::unique_ptr<Object> Clone() const = 0;

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

        virtual std::unique_ptr<Object> Clone() const override final {
            return std::make_unique<Circle>(*this);
        }

    private:
        void RenderObject(const RenderContext& context) const override;

        std::ostream& Print(std::ostream& out) const;

        Point center_;
        double radius_ = 1.0;
    };

    /*
     * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
     * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
     */
    class Polyline final : public Object, public PathProps<Polyline> {
    public:
        using Points = std::vector<Point>;

        Polyline() = default;
        Polyline(Points points) : points_(points) {}

        virtual std::unique_ptr<Object> Clone() const override final {
            return std::make_unique<Polyline>(*this);
        }

        /// Добавляет очередную вершину к ломаной линии
        template <typename Point = svg::Point, detail::EnableIfConvertible<Point, svg::Point> = true>
        Polyline& AddPoint(Point&& point);

    private:
        Points points_;

        std::ostream& PrintPoints(std::ostream& out) const;

        std::ostream& Print(std::ostream& out) const;

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

        virtual std::unique_ptr<Object> Clone() const override final {
            return std::make_unique<Text>(*this);
        }

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

        std::ostream& PrintData(std::ostream& out) const;

        std::ostream& Print(std::ostream& out) const;

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
        constexpr static const std::string_view NEW_LINE{"\n"};

        Document() = default;

        Document(const Document& other) {
            if (this == &other) {
                return;
            }
            assert(objects_.empty());

            CloneObjects(other.objects_);
        }

        Document& operator=(const Document& rhs) {
            if (this == &rhs) {
                return *this;
            }
            CloneObjects(rhs.objects_);
            return *this;
        }

        Document(Document&& other) = default;
        Document& operator=(Document&& rhs) = default;
        virtual ~Document() = default;

        void CloneObjects(const ObjectCollection& other_objects) {
            if (&objects_ == &other_objects) {
                return;
            }
            objects_.reserve(other_objects.size());
            std::for_each(other_objects.begin(), other_objects.end(), [&objects = this->objects_](const auto& other_obj) {
                objects.emplace_back(other_obj->Clone());
            });
        }

        void MoveObjectsFrom(Document&& document) {
            objects_.reserve(objects_.size() + document.objects_.size());
            auto objects = std::move(document.objects_);
            std::move(objects.begin(), objects.end(), std::back_inserter(objects_));
        }

        /// Добавляет в svg-документ объект-наследник svg::Object
        void AddPtr(ObjectPtr&& obj) override;

        /// Выводит в ostream svg-представление документа
        void Render(std::ostream& out) const;

        size_t GetObjectsCount() const {
            return objects_.size();
        }

        void Clear() {
            objects_.clear();
        }

        void Merge(const Document& document) {  //! NOT TESTED
            std::for_each(document.objects_.begin(), document.objects_.end(), [this](const ObjectPtr& object_ptr) {
                using T = std::decay_t<decltype(*object_ptr)>;
                if constexpr (!std::is_abstract_v<T>) {
                    auto copy = object_ptr.get();
                    AddPtr(std::make_unique<T>(std::move(copy)));
                }
            });
        }

    private:
        ObjectCollection objects_;
        static constexpr const std::string_view HEADER_LINE = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv;
        static constexpr const std::string_view SVG_TAG_OPEN = "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv;
        static constexpr const std::string_view SVG_TAG_CLOSE = "</svg>"sv;
    };

}

namespace svg /* Polyline class template implementation */ {
    template <typename Point, detail::EnableIfConvertible<Point, svg::Point>>
    Polyline& Polyline::AddPoint(Point&& point) {
        points_.push_back(std::forward<Point>(point));
        return *this;
    }
}

namespace svg /* Text class template implementation */ {
    template <typename Point, detail::EnableIfConvertible<Point, svg::Point>>
    Text& Text::SetPosition(Point&& pos) {
        base_point_ = std::forward<Point>(pos);
        return *this;
    }

    template <typename Point, detail::EnableIfConvertible<Point, svg::Point>>
    Text& Text::SetOffset(Point&& offset) {
        style_.offset = std::forward<Point>(offset);
        return *this;
    }

    template <typename String, detail::EnableIfConvertible<String, std::string>>
    Text& Text::SetFontFamily(String&& font_family) {
        style_.font_family = std::forward<String>(font_family);
        return *this;
    }

    template <typename String, detail::EnableIfConvertible<String, std::string>>
    Text& Text::SetFontWeight(String&& font_weight) {
        style_.font_weight = std::forward<String>(font_weight);
        return *this;
    }

    template <typename String, detail::EnableIfConvertible<String, std::string>>
    Text& Text::SetData(String&& data) {
        text_ = std::forward<String>(data);
        return *this;
    }
}

namespace svg /* ObjectContainer class template implementation */ {
    template <typename Object, detail::EnableIfBaseOf<svg::Object, Object>>
    void ObjectContainer::Add(Object&& obj) {
        AddPtr(std::make_unique<std::decay_t<Object>>(std::forward<Object>(obj)));
    }
}

namespace svg /* Colors class template implementation */ {
    template <
        typename RgbFrom, typename RgbTo,
        std::enable_if_t<detail::IsConvertible<RgbFrom, Rgb>::value && detail::IsConvertible<RgbTo, Rgb>::value, bool>>
    Rgb Colors::Lerp(RgbFrom&& from, RgbTo to, double t) {
        return {Lerp(from.red, to.red, t), Lerp(from.green, to.green, t), Lerp(from.blue, to.blue, t)};
    }
}

namespace svg /* Colors::ColorPrinter class template implementation */ {

    template <typename String, detail::EnableIfConvertible<String, std::string_view>>
    void Colors::ColorPrinter::operator()(String&& color) const {
        out_ << std::forward<String>(color);
    }

    template <typename RgbType, detail::EnableIfSame<RgbType, Colors::Rgb>>
    void Colors::ColorPrinter::operator()(RgbType&& color) const {
        using namespace std::string_view_literals;
        out_ << "rgb("sv;
        Print(std::forward<RgbType>(color)) << ")"sv;
    }

    template <typename RgbaType, detail::EnableIfSame<RgbaType, Colors::Rgba>>
    void Colors::ColorPrinter::operator()(RgbaType&& color) const {
        using namespace std::string_view_literals;
        out_ << "rgba("sv;
        Print(std::forward<RgbaType>(color)) << ")"sv;
    }

    template <typename RgbType, detail::EnableIfSame<RgbType, svg::Colors::Rgb>>
    std::ostream& svg::Colors::ColorPrinter::Print(RgbType&& color) const {
        using namespace std::string_view_literals;
        out_ << +color.red << SEPARATOR << +color.green << SEPARATOR << +color.blue;
        return out_;
    }

    template <typename RgbaType, detail::EnableIfSame<RgbaType, svg::Colors::Rgba>>
    std::ostream& svg::Colors::ColorPrinter::Print(RgbaType&& color) const {
        using namespace std::string_view_literals;
        Print(static_cast<Rgb>(color)) << SEPARATOR << +color.opacity;
        return out_;
    }
}