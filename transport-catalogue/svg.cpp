#include "svg.h"

namespace svg /* Render class implementation */ {

    using namespace std::literals;

    void Object::Render(const RenderContext& context) const {
        context.RenderIndent();

        // Делегируем вывод тега своим подклассам
        RenderObject(context);

        context.out << "\n"sv;
    }
}

namespace svg /* Circle class implementation */ {

    Circle& Circle::SetCenter(Point center) {
        center_ = center;
        return *this;
    }

    Circle& Circle::SetRadius(double radius) {
        radius_ = radius;
        return *this;
    }

    void Circle::RenderObject(const RenderContext& context) const {
        Print(context.out);
    }

    std::ostream& Circle::Print(std::ostream& out) const {
        out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
        out << "r=\""sv << radius_ << "\""sv;
        RenderAttrs(out);
        out << "/>"sv;

        return out;
    }
}

namespace svg /* Polyline class implementation */ {
    std::ostream& Polyline::PrintPoints(std::ostream& out) const {
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

    std::ostream& Polyline::Print(std::ostream& out) const {
        out << "<polyline points=\""sv;
        PrintPoints(out);
        out << "\""sv;
        RenderAttrs(out);
        out << "/>"sv;
        return out;
    }

    void Polyline::RenderObject(const RenderContext& context) const {
        Print(context.out);
    }
}

namespace svg /* Text class implementation */ {
    Text& Text::SetFontSize(uint32_t size) {
        style_.size = size;
        return *this;
    }

    std::ostream& Text::PrintData(std::ostream& out) const {
        for (const char c : text_) {
            switch (c) {
            case '"':
                out << "&quot;"sv;
                break;
            case '\'':
                out << "&apos;"sv;
                break;
            case '<':
                out << "&lt;"sv;
                break;
            case '>':
                out << "&gt;"sv;
                break;
            case '&':
                out << "&amp;"sv;
                break;
            default:
                out << c;
                break;
            }
        }

        return out;
    }

    std::ostream& Text::Print(std::ostream& out) const {
        out << "<text"sv;
        RenderAttrs(out);
        out << " x=\""sv << base_point_.x << "\" y=\""sv << base_point_.y << "\""sv;
        out << " dx=\""sv << style_.offset.x << "\" dy=\""sv << style_.offset.y << "\""sv;
        out << " font-size=\""sv << style_.size << "\"";

        if (!style_.font_family.empty()) {
            out << " font-family=\""sv << style_.font_family << "\""sv;
        }
        if (!style_.font_weight.empty()) {
            out << " font-weight=\""sv << style_.font_weight << "\""sv;
        }

        out << ">"sv;
        PrintData(out) << "</text>"sv;

        return out;
    }

    void Text::RenderObject(const RenderContext& context) const {
        Print(context.out);
    }
}

namespace svg /* Document class implementation */
{
    Document::Document(const Document& other) {
        if (this == &other) {
            return;
        }
        assert(objects_.empty());

        CloneObjects(other.objects_);
    }

    Document& Document::operator=(const Document& rhs) {
        if (this == &rhs) {
            return *this;
        }
        CloneObjects(rhs.objects_);
        return *this;
    }

    void Document::CloneObjects(const ObjectCollection& other_objects) {
        if (&objects_ == &other_objects) {
            return;
        }
        objects_.reserve(other_objects.size());
        std::for_each(other_objects.begin(), other_objects.end(), [&objects = this->objects_](const auto& other_obj) {
            objects.emplace_back(other_obj->Clone());
        });
    }

    void Document::MoveObjectsFrom(Document&& document) {
        objects_.reserve(objects_.size() + document.objects_.size());
        auto objects = std::move(document.objects_);
        std::move(objects.begin(), objects.end(), std::back_inserter(objects_));
    }

    void Document::AddPtr(ObjectPtr&& obj) {
        objects_.emplace_back(std::move(obj));
    }

    /// Выводит в ostream svg-представление документа
    void Document::Render(std::ostream& out) const {
        out << HEADER_LINE << NEW_LINE << SVG_TAG_OPEN << NEW_LINE;

        for (const auto& obj : objects_) {
            RenderContext ctx(out, 2, 2);
            obj->Render(ctx);
        }

        out << SVG_TAG_CLOSE;
        out.flush();
    }

    size_t Document::GetObjectsCount() const {
        return objects_.size();
    }

    void Document::Clear() {
        objects_.clear();
    }

    void Document::Merge(const Document& document) {
        std::for_each(document.objects_.begin(), document.objects_.end(), [this](const ObjectPtr& object_ptr) {
            using T = std::decay_t<decltype(*object_ptr)>;
            if constexpr (!std::is_abstract_v<T>) {
                auto copy = object_ptr.get();
                AddPtr(std::make_unique<T>(std::move(copy)));
            }
        });
    }
}

namespace svg /* Stroke class implementation */ {
    template <>
    std::string_view Stroke::ToString(StrokeLineCap line_cap) {
        using namespace std::string_view_literals;

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

    template <>
    std::string_view Stroke::ToString(StrokeLineJoin line_join) {
        using namespace std::string_view_literals;

        switch (line_join) {
        case StrokeLineJoin::ARCS:
            return "arcs"sv;
        case StrokeLineJoin::BEVEL:
            return "bevel"sv;
        case StrokeLineJoin::MITER:
            return "miter"sv;
        case StrokeLineJoin::MITER_CLIP:
            return "miter-clip"sv;
        case StrokeLineJoin::ROUND:
            return "round"sv;
        default:
            assert(false);
            break;
        }
        return ""sv;
    }
}

namespace svg /* Colors class implementation */ {
    uint8_t Colors::Lerp(uint8_t from, uint8_t to, double t) {
        return static_cast<uint8_t>(std::round((to - from) * t + from));
    }
}

namespace svg /* Colors::ColorPrinter class implementation */ {

    void Colors::ColorPrinter::operator()(std::monostate) const {
        out_ << Colors::NONE;
    }

    std::ostream& Colors::ColorPrinter::GetStream() const {
        return out_;
    }
}