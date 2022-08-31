#include "domain.h"

namespace transport_catalogue::data /* Stop implementation */ {

    bool Stop::operator==(const Stop& rhs) const noexcept {
        return this == &rhs || (name == rhs.name && coordinates == rhs.coordinates);
    }

    bool Stop::operator!=(const Stop& rhs) const noexcept {
        return !(*this == rhs);
    }
}

namespace transport_catalogue::data /* Bus implementation */ {

    bool Bus::operator==(const Bus& rhs) const noexcept {
        return this == &rhs || (name == rhs.name && route == rhs.route);
    }

    bool Bus::operator!=(const Bus& rhs) const noexcept {
        return !(*this == rhs);
    }
}

namespace transport_catalogue::data /* Hasher implementation */ {
    size_t Hasher::operator()(const std::pair<const Stop*, const Stop*>& stops) const {
        return this->operator()({stops.first, stops.second});
    }
}