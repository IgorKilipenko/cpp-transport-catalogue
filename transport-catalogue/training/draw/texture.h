#pragma once
#include "common.h"

namespace drawing {
    class Texture {
    public:
        explicit Texture(Image image) : image_(std::move(image)) {}

        Size GetSize() const {
            // Заглушка. Реализуйте метод самостоятельно
            return {static_cast<int>(image_.front().size()), static_cast<int>(image_.size())};
        }

        char GetPixelColor(Point p) const {
            return IsPointInRectangle(p, GetSize()) ? image_[p.y][p.x] : ' ';
        }

    private:
        Image image_;
    };
}