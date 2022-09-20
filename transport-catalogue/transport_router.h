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
//#include "json.h"
#include "router.h"
#include "transport_catalogue.h"

namespace transport_catalogue::router {

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

    class TransportRouter {
    private:
        class IndexMapper {
        public:
            IndexMapper() = default;

            IndexMapper(const data::DatabaseScheme::StopsTable& stops) {
                Init_(stops);
            }

            graph::VertexId GetAt(const data::Stop* stop_ptr) const {
                return indexes_.at(stop_ptr);
            }

            size_t IndexesCount() const {
                return indexes_.size();
            }

            bool IsEmpty() const {
                return indexes_.empty();
            }

        private:
            std::unordered_map<const data::Stop*, size_t> indexes_;

            void Init_(const data::DatabaseScheme::StopsTable& stops) {
                size_t idx = 0;
                std::for_each(stops.begin(), stops.end(), [this, &idx](const data::Stop& stop) {
                    indexes_.emplace(&stop, idx++);
                });
            }
        };

    public:
        TransportRouter(RoutingSettings settings, const data::ITransportDataReader& db_reader) : settings_{settings}, db_reader_(db_reader) {}

        void SetWaitTime(int wait_time);

        void SetVelocity(double velocity);

        int GetWaitTime() const;

        std::optional<RouteInfo> GetRouteInfo(std::string_view from_stop, std::string_view to_stop) const {
            /*RouteInfo::ItemsCollection items;
            items.reserve(edges_.size());
            std::for_each(edges_.begin(), edges_.end(), [&items](const auto& edge_item) {
                const RoutingItemInfo& info = edge_item.second;
                items.emplace_back()
            });*/
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
                RouteInfo::WaitInfo wait_info{raw_info.from_stop, raw_info.bus_wait_time_min};
                RouteInfo::BusInfo bus_info{raw_info.bus_name, raw_info.travel_items_count, raw_info.bus_travel_time};
                items.emplace_back(std::move(bus_info), std::move(wait_info));
            }
            return RouteInfo{edge_info->weight, std::move(items)};
        }

        void Build();

    private:
        RoutingSettings settings_;
        const data::ITransportDataReader& db_reader_;
        // graph::DirectedWeightedGraph<double> graph_;
        std::unordered_map<graph::EdgeId, RoutingItemInfo> edges_;
        std::unique_ptr<graph::Router<double>> raw_router_ptr_;
        graph::DirectedWeightedGraph<double> graph_;
        IndexMapper index_mapper_;

    private:
        void AddRouteEdges_(const data::Bus& bus);
    };
}