#include "serialization.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <vector>

#include "domain.h"
#include "map_renderer.h"
#include "svg.h"
#include "transport_catalogue.pb.h"

namespace transport_catalogue::serialization /* DataConvertor implementation */ {
    template <>
    auto DataConvertor::ConvertToModel(data::StopRecord stop) const {
        StopModel stop_model;
        stop_model.set_name(stop->name);

        CoordinatesModel coordinates;
        coordinates.set_lat(stop->coordinates.lat);
        coordinates.set_lng(stop->coordinates.lng);

        *stop_model.mutable_coordinates() = std::move(coordinates);

        return stop_model;
    }

    template <>
    auto DataConvertor::ConvertToModel(data::BusRecord bus) const {
        BusModel bus_model;
        bus_model.set_name(bus->name);
        bus_model.set_is_roundtrip(bus->is_roundtrip);
        std::for_each(bus->route.begin(), bus->route.end(), [&](data::StopRecord stop) {
            bus_model.add_route(stop->name);
        });

        return bus_model;
    }

    template <>
    auto DataConvertor::ConvertToModel(const data::Bus& bus) const {
        data::BusRecord bus_record = &bus;
        return ConvertToModel(bus_record);
    }

    template <>
    auto DataConvertor::ConvertToModel(const data::Stop& stop) const {
        data::StopRecord stop_record = &stop;
        return ConvertToModel(stop_record);
    }

    template <>
    auto DataConvertor::ConvertToModel(DistanceBetweenStopsItem&& distance_item) const {
        DistancesBetweenStopsModel distance_item_model;
        distance_item_model.set_from_stop(distance_item.from_stop->name);
        distance_item_model.set_to_stop(distance_item.to_stop->name);
        distance_item_model.set_distance(distance_item.distance_between);
        return distance_item_model;
    }

    template <>
    auto DataConvertor::ConvertToModel(const svg::Color& color) const {
        
    }

    template <>
    auto DataConvertor::ConvertToModel(const maps::RenderSettings& settings) const {
        //proto_map_schema::RenderSettings settings_model;
        //settings_model.
    }
}

namespace transport_catalogue::serialization /* Store (serialize) implementation */ {

    void Store::PrepareBuses(TransportDataModel& container) const {
        const data::DatabaseScheme::BusRoutesTable& buses = db_reader_.GetDataReader().GetBusRoutesTable();
        std::for_each(buses.begin(), buses.end(), [&](const data::Bus& bus) {
            *container.add_buses() = convertor_.ConvertToModel(bus);
        });
    }

    void Store::PrepareStops(TransportDataModel& container) const {
        const data::DatabaseScheme::StopsTable& stops = db_reader_.GetDataReader().GetStopsTable();
        std::for_each(stops.begin(), stops.end(), [&](const data::Stop& stop) {
            *container.add_stops() = convertor_.ConvertToModel(stop);
        });
    }

    void Store::PrepareDistances(TransportDataModel& container) const {
        const data::DatabaseScheme::DistanceBetweenStopsTable& distances = db_reader_.GetDataReader().GetDistancesBetweenStops();
        std::for_each(distances.begin(), distances.end(), [&](const data::DatabaseScheme::DistanceBetweenStopsTable::value_type& dist_item) {
            *container.add_distances() = convertor_.ConvertToModel(
                DistanceBetweenStopsItem(dist_item.first.first, dist_item.first.second, dist_item.second.measured_distance));
        });
    }

    TransportDataModel Store::BuildSerializableTransportData() const {
        TransportDataModel data;
        PrepareStops(data);
        PrepareDistances(data);
        PrepareBuses(data);
        return data;
    }

    bool Store::SaveToStorage() {
        if (!db_path_.has_value()) {
            return false;
        }

        std::ofstream out(db_path_.value(), std::ios::binary);

        TransportDataModel data = BuildSerializableTransportData();
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

        TransportDataModel data;

        std::ifstream in(db_path_.value(), std::ios::binary);
        assert(data.ParseFromIstream(&in));

        auto stops = std::move(*data.mutable_stops());
        std::for_each(std::move_iterator(stops.begin()), std::move_iterator(stops.end()), [&](StopModel&& stop) {
            db_writer_.AddStop(std::move(*stop.mutable_name()), {stop.coordinates().lat(), stop.coordinates().lng()});
        });

        auto distances = std::move(*data.mutable_distances());
        std::for_each(
            std::move_iterator(distances.begin()), std::move_iterator(distances.end()), [&](DistancesBetweenStopsModel&& dist_item) {
                db_writer_.SetMeasuredDistance(data::MeasuredRoadDistance(
                    std::move(*dist_item.mutable_from_stop()), std::move(*dist_item.mutable_to_stop()), dist_item.distance()));
            });

        auto buses = std::move(*data.mutable_buses());
        std::for_each(std::move_iterator(buses.begin()), std::move_iterator(buses.end()), [&](BusModel&& bus) {
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