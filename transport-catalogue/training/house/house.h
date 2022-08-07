#include <cassert>
#include <stdexcept>
#include <string>
#include <vector>

namespace trainig::sprint10::house {
    using namespace std;

    class House {
        // Реализуйте самостоятельно
    public:
        House(int length, int width, int height) : length_(length), width_(width), height_(height) {}

        int GetLength() const {
            return length_;
        }

        int GetWidth() const {
            return width_;
        }

        int GetHeight() const {
            return height_;
        }

    private:
        int length_, width_, height_ = 0;
    };

    class Resources {
        // Реализуйте самостоятельно
    public:
        Resources(int brick_count) : brick_count_{brick_count} {}

        void TakeBricks(int count) {
            if (count < 0 || brick_count_ - count < 0) {
                throw out_of_range("Count of bricks exception");
            }
            brick_count_ -= count;
        }

        int GetBrickCount() const {
            return brick_count_;
        }

    private:
        int brick_count_;
    };

    struct HouseSpecification {
        int length = 0;
        int width = 0;
        int height = 0;
    };

    class Builder {
        // Реализуйте самостоятельно
    public:
        Builder(Resources& resources) : resources_{resources} {}

        House BuildHouse(HouseSpecification spec) {
            House house(spec.length, spec.width, spec.height);
            int brick_needs = (spec.length + spec.width) * 2 * spec.height * 32;
            if (resources_.GetBrickCount() < brick_needs) {
                throw runtime_error("Need more resources");
            }
            resources_.TakeBricks(brick_needs);
            return house;
        }

    private:
        Resources& resources_;
    };
}