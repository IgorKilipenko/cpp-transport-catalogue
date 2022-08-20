#pragma once

#include <cmath>
#include <iostream>
#include <type_traits>

namespace transport_catalogue::geo {
    [[maybe_unused]]static constexpr double PI() {
        return std::atan(1) * 4.;
    }
    static const double EARTH_RADIUS = 6371000.;

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
}