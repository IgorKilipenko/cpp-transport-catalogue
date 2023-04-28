#include "transport_catalogue.h"

#include <algorithm>
#include <string_view>
#include <utility>

namespace transport_catalogue /* TransportCatalogue implementation */ {

    const std::shared_ptr<TransportCatalogue::Database> TransportCatalogue::GetDatabaseForWrite() const {
        return db_;
    }

    const std::shared_ptr<const TransportCatalogue::Database> TransportCatalogue::GetDatabaseReadOnly() const {
        return db_;
    }

    const data::ITransportDataWriter& TransportCatalogue::GetDataWriter() const {
        return db_writer_;
    }

    const data::ITransportDataReader& TransportCatalogue::GetDataReader() const {
        return db_reader_;
    }

    const data::ITransportStatDataReader& TransportCatalogue::GetStatDataReader() const {
        return *db_stat_reader_;
    }
}

namespace transport_catalogue /* TransportCatalogue < ITransportDataReader implementation */ {

    const data::Bus* TransportCatalogue::GetBus(const std::string_view name) const {
        return db_reader_.GetBus(std::move(name));
    }

    const data::Stop* TransportCatalogue::GetStop(const std::string_view name) const {
        return db_reader_.GetStop(std::move(name));
    }

    const data::DatabaseScheme::StopsTable& TransportCatalogue::GetStopsTable() const {
        return db_reader_.GetStopsTable();
    }

    const data::DatabaseScheme::BusRoutesTable& TransportCatalogue::GetBusRoutesTable() const {
        return db_reader_.GetBusRoutesTable();
    }

    std::vector<data::BusRecord> TransportCatalogue::GetBuses() const {
        return db_reader_.GetBuses();
    }

    const data::BusRecordSet& TransportCatalogue::GetBuses(data::StopRecord stop) const {
        return db_reader_.GetBuses(stop);
    }

    const data::BusRecordSet& TransportCatalogue::GetBuses(const std::string_view bus_name) const {
        return db_reader_.GetBuses(bus_name);
    }

    data::DistanceBetweenStopsRecord TransportCatalogue::GetDistanceBetweenStops(data::StopRecord from, data::StopRecord to) const {
        return db_reader_.GetDistanceBetweenStops(from, to);
    }

    const data::DatabaseScheme::DistanceBetweenStopsTable& TransportCatalogue::GetDistancesBetweenStops() const {
        return db_reader_.GetDistancesBetweenStops();
    }
}

namespace transport_catalogue /* TransportCatalogue < ITransportDataWriter implementation */ {

    void TransportCatalogue::AddStop(data::Stop&& stop) const {
        db_writer_.AddStop(std::move(stop));
    }

    void TransportCatalogue::AddStop(std::string&& name, Coordinates&& coordinates) const {
        db_writer_.AddStop(std::move(name), std::move(coordinates));
    }

    void TransportCatalogue::AddBus(data::Bus&& bus) const {
        db_writer_.AddBus(std::move(bus));
    }

    void TransportCatalogue::AddBus(std::string&& name, const std::vector<std::string_view>& stops, bool is_roundtrip) const {
        db_writer_.AddBus(std::move(name), stops, is_roundtrip);
    }

    void TransportCatalogue::AddBus(std::string&& name, std::vector<std::string>&& stops, bool is_roundtrip) const {
        db_writer_.AddBus(std::move(name), std::move(stops), is_roundtrip);
    }

    void TransportCatalogue::SetMeasuredDistance(const std::string_view from_stop_name, const std::string_view to_stop_name, double distance) const {
        db_writer_.SetMeasuredDistance(from_stop_name, to_stop_name, distance);
    }

    void TransportCatalogue::SetMeasuredDistance(data::MeasuredRoadDistance&& distance) const {
        db_writer_.SetMeasuredDistance(std::move(distance));
    }
}

namespace transport_catalogue /* TransportCatalogue < ITransportStatDataReader implementation */ {

    data::BusStat TransportCatalogue::GetBusInfo(data::BusRecord bus) const {
        return db_stat_reader_->GetBusInfo(bus);
    }

    std::optional<data::BusStat> TransportCatalogue::GetBusInfo(const std::string_view bus_name) const {
        return db_stat_reader_->GetBusInfo(bus_name);
    }

    data::StopStat TransportCatalogue::GetStopInfo(const data::StopRecord stop) const {
        return db_stat_reader_->GetStopInfo(stop);
    }

    std::optional<data::StopStat> TransportCatalogue::GetStopInfo(const std::string_view stop_name) const {
        return db_stat_reader_->GetStopInfo(stop_name);
    }
}

namespace transport_catalogue /* TransportCatalogue::StatReader implementation */ {

    data::BusStat TransportCatalogue::StatReader::GetBusInfo(const data::BusRecord bus) const {
        data::BusStat info;
        double route_length = 0;
        double pseudo_length = 0;
        const data::Route& route = bus->route;

        for (size_t i = 0; i < route.size() - 1ul; ++i) {
            data::StopRecord from_stop = db_reader_.GetStop(route[i]->name);
            data::StopRecord to_stop = db_reader_.GetStop(route[i + 1]->name);
            assert(from_stop && from_stop);

            const auto& dist_btw = db_reader_.GetDistanceBetweenStops(from_stop, to_stop);
            route_length += dist_btw.measured_distance;
            pseudo_length += dist_btw.distance;
        }

        info.total_stops = route.size();
        info.unique_stops = CalculateUniqueStops(route.begin(), route.end());
        info.route_length = route_length;
        info.route_curvature = route_length / std::max(pseudo_length, 1.);

        return info;
    }

    std::optional<data::BusStat> TransportCatalogue::StatReader::GetBusInfo(const std::string_view bus_name) const {
        data::BusRecord bus = db_reader_.GetBus(bus_name);
        return bus != nullptr ? std::optional{GetBusInfo(bus)} : std::nullopt;
    }

    data::StopStat TransportCatalogue::StatReader::GetStopInfo(const data::StopRecord stop) const {
        const data::BusRecordSet& buses = db_reader_.GetBuses(stop);
        std::vector<std::string> buses_names(buses.size());
        std::transform(buses.begin(), buses.end(), buses_names.begin(), [](const auto& bus) {
            return bus->name;
        });
        return data::StopStat{std::move(buses_names)};
    }

    std::optional<data::StopStat> TransportCatalogue::StatReader::GetStopInfo(const std::string_view stop_name) const {
        const data::StopRecord stop = db_reader_.GetStop(stop_name);
        if (stop == nullptr) {
            return std::nullopt;
        }
        return std::optional<data::StopStat>{GetStopInfo(stop)};
    }

    const data::ITransportDataReader& TransportCatalogue::StatReader::GetDataReader() const {
        return db_reader_;
    }
}