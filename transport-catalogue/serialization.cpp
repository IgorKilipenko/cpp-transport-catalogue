#include "serialization.h"


namespace transport_catalogue::serialization /* Store implementation */ {
    void Store::SerializeBuses(data::BusRecord bus, std::ostream& out) const {
        proto_data_schema::Bus bus_model;
        bus_model.set_name(bus->name);
        bus_model.set_is_roundtrip(bus->is_roundtrip);
        std::for_each(bus->route.begin(), bus->route.end(), [&](data::StopRecord stop) {
            bus_model.add_route(stop->name);
        });
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

    template<>
    auto Store::ConvertToSerializable(const data::Bus& bus) const {
        proto_data_schema::Bus bus_model;
        return bus_model;
    }

    void Store::SerializeStops(data::StopRecord stop, std::ostream& out) {
        proto_data_schema::Stop stop_model;
        stop_model.set_name(stop->name);

        proto_data_schema::Coordinates coordinates;
        coordinates.set_lat(stop->coordinates.lat);
        coordinates.set_lng(stop->coordinates.lng);

        *stop_model.mutable_coordinates() = coordinates;

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
        SerializeBuses(out);
        SerializeStops(out);
        return true;
    }
}