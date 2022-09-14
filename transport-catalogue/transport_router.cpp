#include "transport_router.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "domain.h"

namespace transport_catalogue::router {

    void TransportRouter::SetWaitTime(int wait_time) {
        settings_.bus_wait_time = wait_time;
    }

    void TransportRouter::SetVelocity(double velocity) {
        settings_.bus_velocity = velocity;
    }

    int TransportRouter::GetWaitTime() const {
        return settings_.bus_wait_time;
    }

    void TransportRouter::Build() {
        const auto& buses_table = db_reader_.GetBusRoutesTable();

        graph_ = graph::DirectedWeightedGraph<double>(db_reader_.GetStopsTable().size());
        index_mapper_ = IndexMapper(db_reader_.GetStopsTable());

        std::for_each(buses_table.begin(), buses_table.end(), [this](const auto& bus) {
            AddRouteEdges_(bus);
        });

        raw_router_ptr_ = std::make_unique<graph::Router<double>>(graph_);
    }

    void TransportRouter::AddRouteEdges_(const data::Bus& bus) {
        if (bus.route.size() < 2) {
            return;
        }

        for (size_t i = 0; i + 1 < bus.route.size(); ++i) {
            size_t span = 1;
            double weight = settings_.bus_wait_time;
            const data::StopRecord& from_stop_ptr = bus.route[i];
            for (size_t j = i + 1; j < bus.route.size(); ++j) {
                const data::StopRecord& prev_stop_ptr = bus.route[j - 1];
                const data::StopRecord& to_stop_ptr = bus.route[j];
                if (from_stop_ptr == to_stop_ptr) {
                    continue;
                }

                auto it = db_reader_.GetDistanceBetweenStops(prev_stop_ptr, to_stop_ptr);
                weight += it.measured_distance / 1000.0 / settings_.bus_velocity * 60.0;

                //! auto ege_id = graph.AddEdge({reinterpret_cast<uintptr_t>(from_stop_ptr), reinterpret_cast<uintptr_t>(to_stop_ptr), weight});
                auto ege_id = graph_.AddEdge({index_mapper_.GetAt(from_stop_ptr), index_mapper_.GetAt(to_stop_ptr), weight});

                const RoutingItemInfo info{
                    .bus_name = bus.name,
                    .time_wait = settings_.bus_wait_time,
                    .span_count = span,
                    .from_stop = from_stop_ptr->name,
                    .to_stop = to_stop_ptr->name,
                    .prev_stop = prev_stop_ptr->name};
                edges_.emplace(std::move(ege_id), std::move(info));
                ++span;
            }
        }
    }
}
