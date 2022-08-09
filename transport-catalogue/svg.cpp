#include "svg.h"

namespace svg /* Render class impl */ {

    using namespace std::literals;

    void Object::Render(const RenderContext& context) const {
        context.RenderIndent();

        // Делегируем вывод тега своим подклассам
        RenderObject(context);

        context.out << "\n"sv;
    }
}

namespace svg /* Circle class impl */ {

    Circle& Circle::SetCenter(Point center) {
        center_ = center;
        return *this;
    }

    Circle& Circle::SetRadius(double radius) {
        radius_ = radius;
        return *this;
    }

    void Circle::RenderObject(const RenderContext& context) const {
        ToStream(context.out);
    }

    std::ostream& Circle::ToStream(std::ostream& out) const {
        out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
        out << "r=\""sv << radius_ << "\" "sv;
        out << "/>"sv;

        return out;
    }
}

namespace svg /* Polyline class impl */ {
    std::ostream& Polyline::PointsToStream(std::ostream& out) const {
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

    std::ostream& Polyline::ToStream(std::ostream& out) const {
        out << "<polyline points=\""sv;
        PointsToStream(out);
        out << "\" "sv;
        out << "/>"sv;
        return out;
    }

    void Polyline::RenderObject(const RenderContext& context) const {
        ToStream(context.out);
    }
}

namespace svg /* Text class impl */ {
    Text& Text::SetFontSize(uint32_t size) {
        style_.size = size;
        return *this;
    }

    std::ostream& Text::DataToStream(std::ostream& out) const {
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

    std::ostream& Text::ToStream(std::ostream& out) const {
        out << "<text x=\""sv << base_point_.x << "\" y=\""sv << base_point_.y << "\""sv;
        out << " dx=\""sv << style_.offset.x << "\" dy=\""sv << style_.offset.y << "\""sv;
        out << " font-size=\""sv << style_.size << "\"";

        if (!style_.font_family.empty()) {
            out << " font-family=\""sv << style_.font_family << "\""sv;
        }
        if (!style_.font_weight.empty()) {
            out << " font-weight=\""sv << style_.font_weight << "\""sv;
        }

        out << ">"sv;
        DataToStream(out) << "</text>"sv;

        return out;
    }

    void Text::RenderObject(const RenderContext& context) const {
        ToStream(context.out);
    }
}

namespace svg /* Document class impl */
{
    void Document::AddPtr(ObjectPtr&& obj) {
        objects_.emplace_back(std::move(obj));
    }

    /// Выводит в ostream svg-представление документа
    void Document::Render(std::ostream& out) const {
        out << HEADER_LINE << "\n"sv << SVG_TAG_OPEN << "\n"sv;

        for (const auto& obj : objects_) {
            RenderContext ctx(out, 2, 2);
            obj->Render(ctx);
        }

        out << SVG_TAG_CLOSE;
        out.flush();
    }
}