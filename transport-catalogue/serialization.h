#pragma once

#include <transport_catalogue.pb.h>

#include <algorithm>
#include <filesystem>

#include "detail/type_traits.h"
#include "domain.h"
#include "json.h"
#include "transport_catalogue.h"

namespace transport_catalogue::serialization /* Store */ {

    class Store {
    public:
        Store() = default;
        // explicit Store(json::Dict&& json) : json_(std::move(json)) {}

        template <typename TBus, detail::EnableIfSame<TBus, data::Bus> = true>
        void SerializeBus(TBus&& bus);

        template <typename TStop, detail::EnableIfSame<TStop, data::Stop> = true>
        void SerializeStop(TStop&& stop);

        void SetDbPath(std::filesystem::path path) {
            db_path_ = path;
        }

    private:
        TransportCatalogue catalog_;
        json::Dict json_;
        std::filesystem::path db_path_;
    };

}

namespace transport_catalogue::serialization /* Store implementation */ {
    template <typename TBus, detail::EnableIfSame<TBus, data::Bus>>
    void Store::SerializeBus(TBus&& bus) {
        proto_data_schema::Bus bus_model;
        bus_model.set_name(bus.name);
        bus_model.set_is_roundtrip(bus.is_roundtrip);
        std::for_each(bus.route.begin(), bus.route.end(), [&bus_model](auto&& stop) {
            bus_model.add_route(stop->name);
        });
    }

    template <typename TStop, detail::EnableIfSame<TStop, data::Stop>>
    void Store::SerializeStop(TStop&& stop) {
        proto_data_schema::Stop stop_model;
        stop_model.set_name(stop.name);
    }
}