#pragma once

#include <transport_catalogue.pb.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <ostream>
#include <vector>

#include "detail/type_traits.h"
#include "domain.h"
#include "json.h"
#include "transport_catalogue.h"

namespace transport_catalogue::serialization /* Store */ {

    class Store {
    public:
        Store(const data::ITransportStatDataReader& db_reader, const data::ITransportDataWriter& db_writer)
            : db_reader_{db_reader}, db_writer_{db_writer}, stops_{db_reader.GetDataReader().GetStopsTable()} {}

        Store(const Store&) = delete;
        Store(Store&& other) = delete;
        Store& operator=(const Store&) = delete;
        Store& operator=(Store&&) = delete;

        void SerializeBuses(data::BusRecord bus, std::ostream& out) const;
        void SerializeBuses(const data::Bus& bus, std::ostream& out) const;
        void SerializeBuses(std::ostream& out) const;

        void SerializeStops(data::StopRecord stop, std::ostream& out);
        void SerializeStops(const data::Stop& stop, std::ostream& out);
        void SerializeStops(std::ostream& out);

        void SetDbPath(std::filesystem::path path) {
            db_path_ = path;
        }

        bool SaveToStorage();

    private:
        [[maybe_unused]] const data::ITransportStatDataReader& db_reader_;
        [[maybe_unused]] const data::ITransportDataWriter& db_writer_;
        const data::DatabaseScheme::StopsTable& stops_;
        std::optional<std::filesystem::path> db_path_;
    };

}

namespace transport_catalogue::serialization /* Store implementation */ {
    inline void Store::SerializeBuses(data::BusRecord bus, std::ostream& out) const {
        proto_data_schema::Bus bus_model;
        bus_model.set_name(bus->name);
        bus_model.set_is_roundtrip(bus->is_roundtrip);
        std::for_each(bus->route.begin(), bus->route.end(), [&](data::StopRecord stop) {
            bus_model.add_route(stop->name);
        });
        bus_model.SerializeToOstream(&out);
    }

    inline void Store::SerializeBuses(const data::Bus& bus, std::ostream& out) const {
        data::BusRecord bus_record = &bus;
        SerializeBuses(bus_record, out);
    }

    inline void Store::SerializeBuses(std::ostream& out) const {
        const data::DatabaseScheme::BusRoutesTable& buses = db_reader_.GetDataReader().GetBusRoutesTable();
        std::for_each(buses.begin(), buses.end(), [&](const data::Bus& bus) {
            SerializeBuses(bus, out);
        });
    }

    inline void Store::SerializeStops(data::StopRecord stop, std::ostream& out) {
        proto_data_schema::Stop stop_model;
        stop_model.set_name(stop->name);

        proto_data_schema::Coordinates coordinates;
        coordinates.set_lat(stop->coordinates.lat);
        coordinates.set_lng(stop->coordinates.lng);

        *stop_model.mutable_coordinates() = coordinates;

        stop_model.SerializeToOstream(&out);
    }

    inline void Store::SerializeStops(const data::Stop& stop, std::ostream& out) {
        data::StopRecord stop_record = &stop;
        SerializeStops(stop_record, out);
    }

    inline void Store::SerializeStops(std::ostream& out) {
        const data::DatabaseScheme::StopsTable& stops = db_reader_.GetDataReader().GetStopsTable();
        std::for_each(stops.begin(), stops.end(), [&](const data::Stop& stop) {
            SerializeStops(stop, out);
        });
    }

    inline bool Store::SaveToStorage() {
        if (!db_path_.has_value()) {
            return false;
        }
        std::ofstream out(db_path_.value(), std::ios::binary);
        SerializeBuses(out);
        SerializeStops(out);
        return true;
    }
}