#include "transport_router.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "domain.h"

namespace transport_catalogue::router /* TransportRouter implementation */ {

    void TransportRouter::SetSettings(RoutingSettings settings) {
        settings_ = settings;
    }

    const RoutingSettings& TransportRouter::GetSettings() const {
        return settings_;
    }

    const RoutingGraph& TransportRouter::GetGraph() const {
        return graph_;
    }

    void TransportRouter::SetGraph(RoutingGraph&& graph, RoutingIncidentEdges&& route_edges) {
        ResetGraph();
        graph_ = std::move(graph);
        index_mapper_ = IndexMapper(db_reader_.GetStopsTable());
        edges_ = std::move(route_edges);
        raw_router_ptr_ = std::make_unique<graph::Router<double>>(graph_);
        is_builded_ = true;
    }

    bool TransportRouter::HasGraph() const {
        return is_builded_;
    }

    const RoutingItemInfo& TransportRouter::GetRoutingItem(graph::EdgeId edge_id) const {
        return edges_.at(edge_id);
    }

    std::optional<RouteInfo> TransportRouter::GetRouteInfo(std::string_view from_stop, std::string_view to_stop) const {
        assert(is_builded_ && raw_router_ptr_ != nullptr);

        const data::StopRecord from_stop_record = db_reader_.GetStop(from_stop);
        const data::StopRecord to_stop_record = db_reader_.GetStop(to_stop);
        assert(from_stop_record != nullptr && to_stop_record != nullptr);

        auto edge_info = raw_router_ptr_->BuildRoute(index_mapper_.GetAt(from_stop_record), index_mapper_.GetAt(to_stop_record));
        if (!edge_info.has_value()) {
            return std::nullopt;
        }
        RouteInfo::ItemsCollection items;
        items.reserve(edge_info->edges.size());
        for (graph::EdgeId edge_id : edge_info->edges) {
            RoutingItemInfo raw_info = edges_.at(edge_id);
            RouteInfo::WaitInfo wait_info{raw_info.stop_name, raw_info.bus_wait_time_min};
            RouteInfo::BusInfo bus_info{raw_info.bus_name, raw_info.travel_items_count, raw_info.bus_travel_time};
            items.emplace_back(std::move(bus_info), std::move(wait_info));
        }
        return RouteInfo{edge_info->weight, std::move(items)};
    }

    void TransportRouter::Build() {
        assert(!is_builded_ && raw_router_ptr_ == nullptr);

        const auto& buses_table = db_reader_.GetBusRoutesTable();

        graph_ = RoutingGraph(db_reader_.GetStopsTable().size());
        index_mapper_ = IndexMapper(db_reader_.GetStopsTable());

        std::for_each(buses_table.begin(), buses_table.end(), [this](const auto& bus) {
            AddRouteEdges_(bus);
        });

        raw_router_ptr_ = std::make_unique<graph::Router<double>>(graph_);

        is_builded_ = true;
    }

    void TransportRouter::AddRouteEdges_(const data::Bus& bus) {
        if (bus.route.size() < 2) {
            return;
        }

        const data::Route& route = bus.route;

        for (size_t i = 0; i < route.size() - 1ul; ++i) {
            size_t span = 1;
            double total_travel_time = settings_.bus_wait_time_min;
            const data::StopRecord& from_stop_ptr = route[i];
            for (size_t j = i + 1; j < route.size(); ++j) {
                const data::StopRecord& current_stop_ptr = route[j - 1];
                const data::StopRecord& next_stop_ptr = route[j];
                if (from_stop_ptr == next_stop_ptr) {
                    continue;
                }

                auto it = db_reader_.GetDistanceBetweenStops(current_stop_ptr, next_stop_ptr);
                total_travel_time += it.measured_distance / 1000.0 / settings_.bus_velocity_kmh * 60.0;

                auto ege_id =
                    graph_.AddEdge(RoutingGraph::EdgeType{index_mapper_.GetAt(from_stop_ptr), index_mapper_.GetAt(next_stop_ptr), total_travel_time});

                const RoutingItemInfo info{
                    bus.name, settings_.bus_wait_time_min, total_travel_time - settings_.bus_wait_time_min, span, from_stop_ptr->name,
                };

                edges_.emplace(std::move(ege_id), std::move(info));
                ++span;
            }
        }
    }

    void TransportRouter::ResetGraph() {
        raw_router_ptr_ = nullptr;
        is_builded_ = false;
        graph_ = RoutingGraph();
    }
}

namespace transport_catalogue::router /* TransportRouter::IndexMapper implementation */ {
    TransportRouter::IndexMapper::IndexMapper(const data::DatabaseScheme::StopsTable& stops) {
        Init_(stops);
    }

    graph::VertexId TransportRouter::IndexMapper::GetAt(const data::Stop* stop_ptr) const {
        return indexes_.at(stop_ptr);
    }

    size_t TransportRouter::IndexMapper::IndexesCount() const {
        return indexes_.size();
    }

    bool TransportRouter::IndexMapper::IsEmpty() const {
        return indexes_.empty();
    }

    void TransportRouter::IndexMapper::Init_(const data::DatabaseScheme::StopsTable& stops) {
        size_t idx = 0;
        std::for_each(stops.begin(), stops.end(), [this, &idx](const data::Stop& stop) {
            indexes_.emplace(&stop, idx++);
        });
    }
}