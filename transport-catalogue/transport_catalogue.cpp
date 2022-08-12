#include "transport_catalogue.h"
#include <utility>

namespace transport_catalogue {
    const std::deque<data::Stop>& TransportCatalogue::GetStops() const {
        return db_->GetStopsTable();
    }

    const std::shared_ptr<TransportCatalogue::Database> TransportCatalogue::GetDatabaseForWrite() const {
        return db_;
    }

    const std::shared_ptr<const TransportCatalogue::Database> TransportCatalogue::GetDatabaseReadOnly() const {
        return db_;
    }

    const data::BusStat TransportCatalogue::GetBusInfo(const data::Bus* bus) const {
        return db_->GetBusInfo(bus);
    }

    const data::Bus* TransportCatalogue::GetBus(const std::string_view name) const {
        return db_->GetBus(std::move(name));
    }

    const data::Stop* TransportCatalogue::GetStop(const std::string_view name) const {
        return db_->GetStop(std::move(name));
    }

    const data::Bus& TransportCatalogue::AddBus(data::Bus&& bus) {
        return db_->AddBus(std::move(bus));
    }
}