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
        data::BusStat info;
        double route_length = 0;
        double pseudo_length = 0;
        const data::Route& route = bus->route;

        for (auto i = 0; i < route.size() - 1; ++i) {
            const data::Stop* from_stop = db_reader_.GetStop(route[i]->name);
            const data::Stop* to_stop = db_reader_.GetStop(route[i + 1]->name);
            assert(from_stop && from_stop);

            const auto& [measured_dist, dist] = db_reader_.GetDistanceBetweenStops(from_stop, to_stop);
            route_length += measured_dist;
            pseudo_length += dist;
        }

        info.total_stops = route.size();
        info.unique_stops = std::unordered_set<const data::Stop*>(route.begin(), route.end()).size();
        info.route_length = route_length;
        info.route_curvature = route_length / std::max(pseudo_length, 1.);

        return info;
    }

    const data::Bus* TransportCatalogue::GetBus(const std::string_view name) const {
        return db_->GetBus(std::move(name));
    }

    const data::Stop* TransportCatalogue::GetStop(const std::string_view name) const {
        return db_->GetStop(std::move(name));
    }
}