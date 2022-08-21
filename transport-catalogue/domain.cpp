/*
 * В этом файле вы можете разместить классы/структуры, которые являются частью предметной области
 * (domain) вашего приложения и не зависят от транспортного справочника. Например Автобусные
 * маршруты и Остановки.
 *
 * Их можно было бы разместить и в transport_catalogue.h, однако вынесение их в отдельный
 * заголовочный файл может оказаться полезным, когда дело дойдёт до визуализации карты маршрутов:
 * визуализатор карты (map_renderer) можно будет сделать независящим от транспортного справочника.
 *
 * Если структура вашего приложения не позволяет так сделать, просто оставьте этот файл пустым.
 *
 */

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