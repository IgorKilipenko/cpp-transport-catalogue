#include "serialization.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <vector>

#include "domain.h"
#include "transport_catalogue.pb.h"

namespace transport_catalogue::serialization /* Store (serialize) implementation */ {
    template <>
    auto Store::ConvertToSerializable(data::BusRecord bus) const {
        proto_data_schema::Bus bus_model;
        bus_model.set_name(bus->name);
        bus_model.set_is_roundtrip(bus->is_roundtrip);
        std::for_each(bus->route.begin(), bus->route.end(), [&](data::StopRecord stop) {
            bus_model.add_route(stop->name);
        });

        return bus_model;
    }

    template <>
    auto Store::ConvertToSerializable(const data::Bus& bus) const {
        data::BusRecord bus_record = &bus;
        return ConvertToSerializable(bus_record);
    }

    void Store::SerializeBuses(data::BusRecord bus, std::ostream& out) const {
        proto_data_schema::Bus bus_model = ConvertToSerializable(bus);
        bus_model.SerializeToOstream(&out);
    }

    void Store::SerializeBuses(const data::Bus& bus, std::ostream& out) const {
        data::BusRecord bus_record = &bus;
        SerializeBuses(bus_record, out);
    }

    void Store::SerializeBuses(std::ostream& out) const {
        const data::DatabaseScheme::BusRoutesTable& buses = db_reader_.GetDataReader().GetBusRoutesTable();
        std::for_each(buses.begin(), buses.end(), [&](const data::Bus& bus) {
            SerializeBuses(bus, out);
        });
    }

    template <>
    auto Store::ConvertToSerializable(data::StopRecord stop) const {
        proto_data_schema::Stop stop_model;
        //! stop_model.set_id(reinterpret_cast<uintptr_t>(&stop));
        stop_model.set_name(stop->name);

        proto_data_schema::Coordinates coordinates;
        coordinates.set_lat(stop->coordinates.lat);
        coordinates.set_lng(stop->coordinates.lng);

        *stop_model.mutable_coordinates() = std::move(coordinates);

        return stop_model;
    }

    template <>
    auto Store::ConvertToSerializable(const data::Stop& stop) const {
        data::StopRecord stop_record = &stop;
        return ConvertToSerializable(stop_record);
    }

    void Store::SerializeStops(data::StopRecord stop, std::ostream& out) {
        proto_data_schema::Stop stop_model = ConvertToSerializable(stop);
        stop_model.SerializeToOstream(&out);
    }

    void Store::SerializeStops(const data::Stop& stop, std::ostream& out) {
        data::StopRecord stop_record = &stop;
        SerializeStops(stop_record, out);
    }

    void Store::SerializeStops(std::ostream& out) {
        const data::DatabaseScheme::StopsTable& stops = db_reader_.GetDataReader().GetStopsTable();
        std::for_each(stops.begin(), stops.end(), [&](const data::Stop& stop) {
            SerializeStops(stop, out);
        });
    }

    std::vector<proto_data_schema::Bus> Store::ConvertBusesToSerializable() const {
        const data::DatabaseScheme::BusRoutesTable& buses = db_reader_.GetDataReader().GetBusRoutesTable();
        std::vector<proto_data_schema::Bus> result(buses.size());

        std::transform(buses.begin(), buses.end(), result.begin(), [&](const data::Bus& bus) {
            return ConvertToSerializable(bus);
        });

        return result;
    }

    void Store::ConvertBusesToSerializable(proto_data_schema::TransportData& container) const {
        const data::DatabaseScheme::BusRoutesTable& buses = db_reader_.GetDataReader().GetBusRoutesTable();
        std::for_each(buses.begin(), buses.end(), [&](const data::Bus& bus) {
            *container.add_buses() = ConvertToSerializable(bus);
        });
    }

    void Store::ConvertStopsToSerializable(proto_data_schema::TransportData& container) const {
        const data::DatabaseScheme::StopsTable& stops = db_reader_.GetDataReader().GetStopsTable();
        std::for_each(stops.begin(), stops.end(), [&](const data::Stop& stop) {
            *container.add_stops() = ConvertToSerializable(stop);
        });
    }

    proto_data_schema::TransportData Store::BuildSerializableTransportData() const {
        proto_data_schema::TransportData data;
        ConvertStopsToSerializable(data);
        ConvertBusesToSerializable(data);

        return data;
    }

    bool Store::SaveToStorage() {
        if (!db_path_.has_value()) {
            return false;
        }

        std::ofstream out(db_path_.value(), std::ios::binary);

        proto_data_schema::TransportData data = BuildSerializableTransportData();
        bool success = data.SerializeToOstream(&out);

        assert(success);

        return true;
    }
}

namespace transport_catalogue::serialization /* Store (deserialize) implementation */ {
    bool Store::DeserializeTransportData() const {
        if (!db_path_.has_value()) {
            return false;
        }

        proto_data_schema::TransportData data;

        std::ifstream in(db_path_.value(), std::ios::binary);
        assert(data.ParseFromIstream(&in));

        auto stops = std::move(*data.mutable_stops());
        std::for_each(std::move_iterator(stops.begin()), std::move_iterator(stops.end()), [&](proto_data_schema::Stop&& stop) {
            db_writer_.AddStop(std::move(*stop.mutable_name()), {stop.coordinates().lat(), stop.coordinates().lng()});
        });

        auto buses = std::move(*data.mutable_buses());
        std::for_each(std::move_iterator(buses.begin()), std::move_iterator(buses.end()), [&](proto_data_schema::Bus&& bus) {
            std::string name = std::move(*bus.mutable_name());
            bool is_roundtrip = bus.is_roundtrip();
            std::vector<std::string> stops(bus.route_size());
            std::transform(
                std::move_iterator(bus.mutable_route()->begin()), std::move_iterator(bus.mutable_route()->end()), stops.begin(),
                [](std::string&& name) {
                    return std::move(name);
                });
            db_writer_.AddBus(std::move(name), std::move(stops), is_roundtrip);
        });

        return true;
    }
}