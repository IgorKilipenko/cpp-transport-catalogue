#pragma once
#include <algorithm>
#include <cstddef>
#include <memory>
#include <string_view>

#include "common.h"
#include "texture.h"

namespace drawing {
    // Поддерживаемые виды фигур: прямоугольник и эллипс
    enum class ShapeType { RECTANGLE, ELLIPSE };

    class Shape {
    public:
        // Фигура после создания имеет нулевые координаты и размер,
        // а также не имеет текстуры
        explicit Shape(ShapeType type) : type_{std::move(type)}, position_{}, size_{}, texture_{nullptr} {}

        void SetPosition(Point pos) {
            position_ = pos;
        }

        void SetSize(Size size) {
            size_ = size;
        }

        void SetTexture(std::shared_ptr<Texture> texture) {
            texture_ = texture;
        }

        // Рисует фигуру на указанном изображении
        // В зависимости от типа фигуры должен рисоваться либо эллипс, либо прямоугольник
        // Пиксели фигуры, выходящие за пределы текстуры, а также в случае, когда текстура не задана,
        // должны отображаться с помощью символа точка '.'
        // Части фигуры, выходящие за границы объекта image, должны отбрасываться.
        void Draw(Image& image) const {
            // const Size img_size{static_cast<int>(image.front().size()), static_cast<int>(image.size())};
            //const auto isPointIn = type_ == ShapeType::ELLIPSE ? IsPointInEllipse : IsPointInRectangle;
            static const char empty_pixel = '.';
            image.resize(image.size() + position_.y, image.front());

            int y = -1;
            for (std::string& str : image) {
                str.resize(str.size() + position_.x, str.front());
                if (++y < position_.y) {
                    continue;
                }
                int x = -1;
                for (auto& ch : str) {
                    if (++x < position_.x) {
                        continue;
                    }
                    const Point p{x-position_.x, y-position_.y};
                    ch = !texture_ ? empty_pixel :/* isPointIn(p, size_) ?*/ texture_->GetPixelColor(p) /*: empty_pixel*/;
                }
            }
            /*std::string line (image.front().size() + position_.y, image.front()[0]);
            Image tmp_img(image.size() + position_.x, std::move(line));
            line.resize(tmp_img.size());*/

        }

    private:
        ShapeType type_;
        Point position_;
        Size size_;
        std::shared_ptr<Texture> texture_;
    };
}