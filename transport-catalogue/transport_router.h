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

namespace transport_catalogue::router /* Types */ {
    using RoutingGraph = graph::DirectedWeightedGraph<double>;
}

namespace transport_catalogue::router /* RouteInfo */ {

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
        std::string_view stop_name;
    };

    struct RoutingSettings {
        double bus_wait_time_min = 0;
        double bus_velocity_kmh = 0.0;
    };
}

namespace transport_catalogue::router /* TransportRouter interface */ {
    class ITransportRouter {
    public:
        virtual void SetSettings(RoutingSettings settings) = 0;
        virtual const RoutingSettings& GetSettings() const = 0;

        virtual const RoutingGraph& GetGraph() const = 0;
        virtual void SetGraph(RoutingGraph&& graph) = 0;

        virtual bool HasGraph() const = 0;
        virtual const RoutingItemInfo& GetRoutingItem(graph::EdgeId edge_id) const = 0;
    };
}

namespace transport_catalogue::router /* TransportRouter */ {

    class TransportRouter : public ITransportRouter {
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

        void SetSettings(RoutingSettings settings) override;

        const RoutingSettings& GetSettings() const override;

        std::optional<RouteInfo> GetRouteInfo(std::string_view from_stop, std::string_view to_stop) const;
        void Build();

        const RoutingItemInfo& GetRoutingItem(graph::EdgeId edge_id) const override;
        const RoutingGraph& GetGraph() const override;
        void SetGraph(RoutingGraph&& graph) override;
        virtual bool HasGraph() const override;

        void ResetGraph();

    private:
        RoutingSettings settings_;
        const data::ITransportDataReader& db_reader_;
        std::unordered_map<graph::EdgeId, RoutingItemInfo> edges_;
        std::unique_ptr<graph::Router<double>> raw_router_ptr_;
        RoutingGraph graph_;
        IndexMapper index_mapper_;
        bool is_builded_ = false;

    private:
        void AddRouteEdges_(const data::Bus& bus);
    };
}