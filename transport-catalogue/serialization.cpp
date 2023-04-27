#include "serialization.h"

#include "domain.h"

namespace transport_catalogue::serialization /* Store implementation */ {
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
        stop_model.set_name(stop->name);

        proto_data_schema::Coordinates coordinates;
        coordinates.set_lat(stop->coordinates.lat);
        coordinates.set_lng(stop->coordinates.lng);

        *stop_model.mutable_coordinates() = coordinates;

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

    bool Store::SaveToStorage() {
        if (!db_path_.has_value()) {
            return false;
        }
        std::ofstream out(db_path_.value(), std::ios::binary);
        SerializeStops(out);
        SerializeBuses(out);
        return true;
    }
}