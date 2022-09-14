#pragma once

#include <cstddef>
#include <memory>
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
            std::string_view bus{""};
            size_t span_count = 0;
            time_t time = 0;
        };

        struct WaitInfo {
            std::string_view stop_name{""};
            time_t time = 0;
        };

        time_t total_time;
        std::vector<std::variant<BusInfo, WaitInfo>> items;
    };

    struct RoutingItemInfo {
        std::string_view bus_name;
        time_t time_wait = 0;
        size_t span_count = 0;
        std::string_view from_stop;
        std::string_view to_stop;
        std::string_view prev_stop;
    };

    struct RoutingSettings {
        int bus_wait_time = 0;
        double bus_velocity = 0.0;
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