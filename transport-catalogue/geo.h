#pragma once

#include <cmath>
#include <exception>
#include <execution>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace transport_catalogue::detail /* template helpers */ {
    template <bool Condition>
    using EnableIf = typename std::enable_if_t<Condition, bool>;

    template <typename FromType, typename ToType>
    using EnableIfConvertible = std::enable_if_t<std::is_convertible_v<std::decay_t<FromType>, ToType>, bool>;

    template <typename FromType, typename ToType>
    using IsConvertible = std::is_convertible<std::decay_t<FromType>, ToType>;

    template <typename FromType, typename ToType>
    inline constexpr bool IsConvertibleV = std::is_convertible<std::decay_t<FromType>, ToType>::value;

    template <typename FromType, typename ToType>
    using IsSame = std::is_same<std::decay_t<FromType>, ToType>;

    template <typename FromType, typename ToType>
    inline constexpr bool IsSameV = std::is_same<std::decay_t<FromType>, ToType>::value;

    template <typename FromType, typename ToType>
    using EnableIfSame = std::enable_if_t<std::is_same_v<std::decay_t<FromType>, ToType>, bool>;

    template <typename BaseType, typename DerivedType>
    using IsBaseOf = std::is_base_of<BaseType, std::decay_t<DerivedType>>;

    template <typename FromType, typename ToType>
    inline constexpr bool IsBaseOfV = IsBaseOf<FromType, ToType>::value;

    template <typename BaseType, typename DerivedType>
    using EnableIfBaseOf = std::enable_if_t<std::is_base_of_v<BaseType, std::decay_t<DerivedType>>, bool>;

    template <class ExecutionPolicy>
    using IsExecutionPolicy = std::is_execution_policy<std::decay_t<ExecutionPolicy>>;

    template <class ExecutionPolicy>
    using EnableForExecutionPolicy = typename std::enable_if_t<IsExecutionPolicy<ExecutionPolicy>::value, bool>;
}

namespace transport_catalogue::geo {

    static const double EARTH_RADIUS = 6371000.;
    static constexpr const double THRESHOLD = 1e-6;

    struct Coordinates {
        double lat = 0.;
        double lng = 0.;

        bool operator==(const Coordinates& other) const {
            return lat == other.lat && lng == other.lng;
        }

        bool operator!=(const Coordinates& other) const {
            return !(*this == other);
        }

        Coordinates() : Coordinates(0., 0.) {}

        Coordinates(double latitude, double longitude) : lat{latitude}, lng{longitude} {}

        Coordinates(Coordinates&& other) : lat{std::move(other.lat)}, lng{std::move(other.lng)} {}

        Coordinates(const Coordinates& other) = default;

        Coordinates& operator=(const Coordinates& other) = default;

        Coordinates& operator=(Coordinates&& other) = default;
    };

    double ComputeDistance(Coordinates from, Coordinates to);

    struct Point {
        double north = 0.;
        double east = 0.;
        Point() = default;
        Point(double north, double east) : north(north), east(east) {}
        virtual ~Point() = default;
    };

    struct Offset : Point {
        using Point::Point;
    };

    struct Size {
        Size() = default;
        Size(double height, double width) : height(height), width(width) {}
        double height = 0.;
        double width = 0.;
    };
}

namespace transport_catalogue::geo {
    template <typename CoordinatesType = geo::Coordinates>
    struct Bounds {
        CoordinatesType min;
        CoordinatesType max;

        Bounds() = default;

        template <
            typename Coordinates = geo::Coordinates,
            detail::EnableIf<detail::IsBaseOfV<geo::Coordinates, Coordinates> || detail::IsBaseOfV<geo::Point, Coordinates>> = true>
        Bounds(Coordinates&& min, Coordinates&& max) : min(std::forward<Coordinates>(min)), max(std::forward<Coordinates>(max)) {}
    };

    class Projection {
    public:
        template <typename ProjectionType>
        static ProjectionType GetProjection(const std::string& projection_name = "SphereProjection");

        virtual Bounds<> GetBounds() const {
            return {};
        }

        virtual Point FromLatLngToMapPoint([[maybe_unused]] Coordinates lat_lng) const {
            return {lat_lng.lat, lat_lng.lng};
        }

        virtual double GetEarsRadius() const {
            return 0.;
        }

        virtual bool operator==(const Projection& rhs) const {
            return this == &rhs || code_ == rhs.code_;
        }

        virtual bool operator!=(const Projection& rhs) const {
            return !(*this == rhs);
        }

    protected:
        Projection(std::string unique_code) : code_(unique_code) {}

    private:
        std::string code_;
    };
}

namespace transport_catalogue::geo {
    class SphereProjection : public Projection {
    public:
        struct ScaleFactor {
            std::optional<double> height = 0.;
            std::optional<double> width = 0.;
        };

    public:
        SphereProjection() : SphereProjection(Bounds<>{}, 0, 0) {}
        SphereProjection(Bounds<> bounds, double scale, double padding)
            : Projection("SphereProjection"), bounds_{bounds}, scale_{scale}, padding_(padding) {}

        SphereProjection(const SphereProjection& other) = default;
        SphereProjection& operator=(const SphereProjection& other) {
            if (this == &other) {
                return *this;
            }
            this->bounds_ = other.bounds_;
            this->scale_ = other.scale_;
            this->padding_ = other.padding_;

            return *this;
        }
        SphereProjection(SphereProjection&& other) = default;
        SphereProjection& operator=(SphereProjection&& other) {
            if (this == &other) {
                return *this;
            }

            this->bounds_ = std::move(other.bounds_);
            this->scale_ = std::move(other.scale_);
            this->padding_ = std::move(other.padding_);

            return *this;
        }

        bool operator==(const SphereProjection& rhs) const {
            return this == &rhs || (rhs.bounds_.max == this->bounds_.max && rhs.bounds_.min == this->bounds_.min && rhs.scale_ == this->scale_);
        }

        bool operator!=(const SphereProjection& rhs) const {
            return !(*this == rhs);
        }

        template <template <typename, typename...> class Container, typename Coordinates, typename... Types>
        static SphereProjection CalculateFromParams(Container<Coordinates, Types...> points, Size map_size, double padding) {
            Bounds<> bounds = CalculateBounds(points.begin(), points.end());
            double scale = CalculateScale(std::move(map_size), bounds, padding);
            return SphereProjection(std::move(bounds), scale, padding);
        }

        Bounds<> GetBounds() const override {
            return bounds_;
        }

        double GetEarsRadius() const override {
            return radius_;
        }

        Point FromLatLngToMapPoint(Coordinates coordinates) const override {
            return {(coordinates.lng - bounds_.min.lng) * scale_ + padding_, (bounds_.max.lat - coordinates.lat) * scale_ + padding_};
        }

    private:
        Bounds<> bounds_;
        double scale_ = 0.;
        double padding_ = 0.;
        inline static const double radius_ = EARTH_RADIUS;

        static bool IsZero(double value) {
            return std::abs(value) < THRESHOLD;
        }

        static double CalculateScale(Size map_size, const Bounds<>& bounds, double padding) {
            double zoom = 0.;
            ScaleFactor scale_factor;
            const auto [min_lat, min_lng] = bounds.min;
            const auto [max_lat, max_lng] = bounds.max;

            if (!IsZero(max_lng - min_lng)) {
                scale_factor.width = (map_size.width - 2 * padding) / (max_lng - min_lng);
            }

            if (!IsZero(max_lat - min_lat)) {
                scale_factor.height = (map_size.height - 2 * padding) / (max_lat - min_lat);
            }

            if (scale_factor.width && scale_factor.height) {
                zoom = std::min(*scale_factor.width, *scale_factor.height);
            } else if (scale_factor.width) {
                zoom = *scale_factor.width;
            } else if (scale_factor.height) {
                zoom = *scale_factor.height;
            }

            return zoom;
        }

        template <typename PointInputIt>
        static Bounds<> CalculateBounds(PointInputIt points_begin, PointInputIt points_end) {
            if (points_begin == points_end) {
                return {};
            }

            const auto [left_it, right_it] = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) {
                return lhs.lng < rhs.lng;
            });
            const double min_lng = left_it->lng;
            const double max_lng = right_it->lng;

            const auto [bottom_it, top_it] = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) {
                return lhs.lat < rhs.lat;
            });
            const double min_lat = bottom_it->lat;
            const double max_lat = top_it->lat;

            return {{min_lat, min_lng}, {max_lat, max_lng}};
        }
    };
}