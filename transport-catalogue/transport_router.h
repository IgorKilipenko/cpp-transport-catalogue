#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "domain.h"
#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"

namespace transport_catalogue::router /* RouteInfo */{

    class RouteInfo {
    public:
        struct BusInfo {
            inline static const std::string TYPE_NAME = "Bus";
            std::string_view bus{""};
            size_t span_count = 0;
            double time = 0;
        };

        struct WaitInfo {
            inline static const std::string TYPE_NAME = "Wait";
            std::string_view stop_name{""};
            double time = 0;
        };

        using Item = std::pair<BusInfo, WaitInfo>;
        using ItemsCollection = std::vector<Item>;

        double total_time;
        ItemsCollection items;
    };

    struct RoutingItemInfo {
        std::string_view bus_name;
        double bus_wait_time_min = 0.;
        double bus_travel_time = 0.;
        size_t travel_items_count = 0;
        std::string_view from_stop;
        std::string_view next_stop;
        std::string_view current_stop;
    };

    struct RoutingSettings {
        double bus_wait_time_min = 0;
        double bus_velocity_kmh = 0.0;
    };
}

namespace transport_catalogue::router /* TransportRouter */ {

    class TransportRouter {
    private:
        class IndexMapper {
        public:
            IndexMapper() = default;
            IndexMapper(const data::DatabaseScheme::StopsTable& stops);

            graph::VertexId GetAt(const data::Stop* stop_ptr) const;
            size_t IndexesCount() const;
            bool IsEmpty() const;

        private:
            std::unordered_map<const data::Stop*, size_t> indexes_;

        private:
            void Init_(const data::DatabaseScheme::StopsTable& stops);
        };

    public:
        TransportRouter(RoutingSettings settings, const data::ITransportDataReader& db_reader) : settings_{settings}, db_reader_(db_reader) {}

        void SetSettings(RoutingSettings settings);

        const RoutingSettings& GetSettings() const;

        std::optional<RouteInfo> GetRouteInfo(std::string_view from_stop, std::string_view to_stop) const;
        void Build();

    private:
        RoutingSettings settings_;
        const data::ITransportDataReader& db_reader_;
        std::unordered_map<graph::EdgeId, RoutingItemInfo> edges_;
        std::unique_ptr<graph::Router<double>> raw_router_ptr_;
        graph::DirectedWeightedGraph<double> graph_;
        IndexMapper index_mapper_;

    private:
        void AddRouteEdges_(const data::Bus& bus);
    };
}