#pragma once

#include <deque>
#include <memory>
#include <string>

#include "domain.h"

namespace transport_catalogue {
    using Coordinates = data::Coordinates;

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
        const data::Stop& AddStop(String&& name, Coordinates&& coordinates);

        const data::Bus* GetBus(const std::string_view name) const;

        const data::Stop* GetStop(const std::string_view name) const;

        const std::deque<data::Stop>& GetStops() const;

        const std::shared_ptr<Database> GetDatabaseForWrite() const;

        const std::shared_ptr<const Database> GetDatabaseReadOnly() const;

        const data::BusStat GetBusInfo(const data::Bus* bus) const;

    private:
        std::shared_ptr<Database> db_;
    };

}

namespace transport_catalogue {
    template <
        typename String, typename Coordinates,
        std::enable_if_t<
            std::is_same_v<std::decay_t<String>, std::string> && std::is_same_v<std::decay_t<Coordinates>, transport_catalogue::Coordinates>, bool>>
    const data::Stop& TransportCatalogue::AddStop(String&& name, Coordinates&& coordinates) {
        return db_->AddStop(std::move(name), std::move(coordinates));
    }
}
