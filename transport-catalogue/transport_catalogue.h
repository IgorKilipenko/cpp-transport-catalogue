#pragma once

#include <deque>
#include <memory>
#include <string>

#include "domain.h"

namespace transport_catalogue {
    using Coordinates = data::Coordinates;
    using Stop = data::Stop;
    using Bus = data::Bus;
    using Route = data::Route;
    using BusInfo = data::BusRouteInfo;

    class TransportCatalogue {
    public:
        using Database = data::Database<TransportCatalogue>;
        TransportCatalogue() : db_{new Database()} {}
        TransportCatalogue(std::shared_ptr<Database> db) : db_{db} {}

        template <
            typename String, typename Coordinates,
            std::enable_if_t<
                std::is_same_v<std::decay_t<String>, std::string> && std::is_same_v<std::decay_t<Coordinates>, transport_catalogue::Coordinates>,
                bool> = true>
        const Stop& AddStop(String&& name, Coordinates&& coordinates);

        const Bus* GetBus(const std::string_view name) const;

        const Stop* GetStop(const std::string_view name) const;

        const std::deque<Stop>& GetStops() const;

        const std::shared_ptr<Database> GetDatabaseForWrite() const;

        const std::shared_ptr<const Database> GetDatabaseReadOnly() const;

        const BusInfo GetBusInfo(const Bus* bus) const;

    private:
        std::shared_ptr<Database> db_;
    };

}

namespace transport_catalogue {
    template <
        typename String, typename Coordinates,
        std::enable_if_t<
            std::is_same_v<std::decay_t<String>, std::string> && std::is_same_v<std::decay_t<Coordinates>, transport_catalogue::Coordinates>, bool>>
    const Stop& TransportCatalogue::AddStop(String&& name, Coordinates&& coordinates) {
        return db_->AddStop(std::move(name), std::move(coordinates));
    }
}
