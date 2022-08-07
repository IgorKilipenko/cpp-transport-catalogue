#pragma once
#include "common.h"

namespace drawing {
    class Texture {
    public:
        explicit Texture(Image image) : image_(std::move(image)) {}

        Size GetSize() const {
            // Заглушка. Реализуйте метод самостоятельно
             return GetImageSize(image_);
        }

        char GetPixelColor(Point p) const {
            return image_[p.y][p.x];
        }

    private:
        Image image_;
    };
}