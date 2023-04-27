#pragma once

#include <transport_catalogue.pb.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
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

        void SerializeBuses(std::ostream& out) const;

        template <typename TStop, detail::EnableIfSame<TStop, data::Stop> = true>
        proto_data_schema::Stop SerializeStop(TStop&& stop);

        void SetDbPath(std::filesystem::path path) {
            db_path_ = path;
        }

        void SaveToStorage();

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

    inline void Store::SerializeBuses(std::ostream& out) const {
        std::vector<data::BusRecord> buses = db_reader_.GetDataReader().GetBuses();
        std::for_each(buses.begin(), buses.end(), [&](data::BusRecord bus) {
            SerializeBuses(bus, out);
        });
    }

    template <typename TStop, detail::EnableIfSame<TStop, data::Stop>>
    proto_data_schema::Stop Store::SerializeStop(TStop&& stop) {
        proto_data_schema::Stop stop_model;
        stop_model.set_name(stop.name);
    }

    inline void Store::SaveToStorage() {
        if (!db_path_.has_value()) {
            return;
        }
        std::ofstream out(db_path_.value(), std::ios::binary);
        SerializeBuses(out);
    }
}