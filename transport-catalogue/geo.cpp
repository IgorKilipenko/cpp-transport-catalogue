#include "geo.h"

namespace transport_catalogue::geo {
    double ComputeDistance(Coordinates from, Coordinates to) {
        using namespace std;
        if (from == to) {
            return 0;
        }
        static const double dr = PI() / 180.;
        return std::acos(
                   std::sin(from.lat * dr) * std::sin(to.lat * dr) +
                   std::cos(from.lat * dr) * std::cos(to.lat * dr) * std::cos(std::abs(from.lng - to.lng) * dr)) *
               EARTH_RADIUS;
    }
}