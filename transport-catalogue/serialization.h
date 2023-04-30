#pragma once

#include <transport_catalogue.pb.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <type_traits>
#include <vector>

#include "domain.h"
#include "graph.pb.h"
#include "map_renderer.h"
#include "map_renderer.pb.h"
#include "transport_router.h"

namespace transport_catalogue::serialization /* Common model types aliases */ {
    using ColorModel = proto_schema::svg::Color;
    using DatabaseModel = proto_schema::transport::Database;
}

namespace transport_catalogue::serialization /* Transport data model types aliases */ {
    using StopModel = proto_schema::transport::Stop;
    using BusModel = proto_schema::transport::Bus;
    using CoordinatesModel = proto_schema::transport::Coordinates;
    using DistancesBetweenStopsModel = proto_schema::transport::DistancesBetweenStops;
    using TransportDataModel = proto_schema::transport::TransportData;
    struct DistanceBetweenStopsItem {
        DistanceBetweenStopsItem(data::StopRecord from_stop, data::StopRecord to_stop, double distance_between)
            : from_stop{from_stop}, to_stop{to_stop}, distance_between{distance_between} {}
        data::StopRecord from_stop;
        data::StopRecord to_stop;
        double distance_between;
    };
}

namespace transport_catalogue::serialization /* Settings types aliases and defs */ {
    using RenderSettingsModel = proto_schema::maps::RenderSettings;
    using RoutingSettingsModel = proto_schema::router::RoutingSettings;
    using SettingsModel = proto_schema::transport::Settings;
}

namespace transport_catalogue::serialization /* Routings types aliases and defs */ {
    using RoutingGraphModel = proto_schema::graph::RoutingGraph;
    using EdgeModel = proto_schema::graph::Edge;
    using IncidentEdgesModel = proto_schema::graph::IncidentEdges;
    using RouterModel = proto_schema::router::Router;
}

namespace transport_catalogue::serialization /* DataConvertor */ {
    class DataConverter {
    public:
        template <typename T, std::enable_if_t<!std::is_pointer_v<std::decay_t<T>>, bool> = true>
        auto ConvertToModel(T&& object) const;

        template <typename T, std::enable_if_t<std::is_same_v<std::decay_t<T>, data::DbRecord<std::remove_pointer_t<std::decay_t<T>>>>, bool> = true>
        auto ConvertToModel(T object_ptr) const;

        template <typename T>
        auto ConvertFromModel(T&& object) const;
    };
}

namespace transport_catalogue::serialization /* Store */ {
    class Store {
    public: /* constructors */
        Store(
            const data::ITransportStatDataReader& db_reader, const data::ITransportDataWriter& db_writer, io::renderer::IRenderer& map_renderer,
            router::ITransportRouter& transport_router)
            : db_reader_{db_reader}, db_writer_{db_writer}, map_renderer_{map_renderer}, transport_router_{transport_router}, converter_{} {}

        Store(const Store&) = delete;
        Store(Store&& other) = delete;
        Store& operator=(const Store&) = delete;
        Store& operator=(Store&&) = delete;

    public: /* serialize methods */
        void SetDbPath(std::filesystem::path path) {
            db_path_ = path;
        }

        /// Transport data serialization
        void PrepareBuses(TransportDataModel& data_model) const;
        void PrepareStops(TransportDataModel& data_model) const;
        void PrepareDistances(TransportDataModel& data_model) const;
        TransportDataModel BuildSerializableTransportData() const;

        /// App settings serialization
        void PrepareRenderSettings(SettingsModel& settings_model) const;
        void PrepareRoutingSettings(SettingsModel& settings_model) const;
        SettingsModel BuildSerializableSettings() const;

        void PrepareGraphModel(RouterModel& router_model) const;
        void PrepareRouterModel(RouterModel& router_model) const;
        RouterModel BuildSerializableRouterModel() const;

        bool SaveToStorage();

    public: /* deserialize methods */
        void FillTransportData(TransportDataModel&& data_model) const;
        void FillRenderSettings(RenderSettingsModel&& render_settings_model) const;
        void FillRoutingSettings(RoutingSettingsModel&& routing_settings_model) const;
        void FillSettings(SettingsModel&& settings_model) const;
        bool DeserializeDatabase() const;

    private:
        const data::ITransportStatDataReader& db_reader_;
        const data::ITransportDataWriter& db_writer_;
        io::renderer::IRenderer& map_renderer_;
        router::ITransportRouter& transport_router_;

        std::optional<std::filesystem::path> db_path_;
        const DataConverter converter_;
    };

}