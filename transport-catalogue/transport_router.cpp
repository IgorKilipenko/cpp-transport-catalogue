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
        settings_.bus_wait_time_min = wait_time;
    }

    void TransportRouter::SetVelocity(double velocity) {
        settings_.bus_velocity_kmh = velocity;
    }

    int TransportRouter::GetWaitTime() const {
        return settings_.bus_wait_time_min;
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

        const auto& last_stop_of_route = bus.GetLastStopOfRoute();
        /*auto last_it = std::find(bus.route.begin(), bus.route.end(), last_stop_of_route);
        data::Route route{bus.route.begin(), !bus.is_roundtrip ? std::next(last_it) : bus.route.end()};
        if (!bus.is_roundtrip) {
            route.insert(route.end(), last_it,  bus.route.end());
        }*/

        const data::Route& route = bus.route;

        for (size_t i = 0; i < route.size() - 1ul; ++i) {
            size_t span = 1;
            double total_travel_time = settings_.bus_wait_time_min;  //! waigth
            const data::StopRecord& from_stop_ptr = route[i];
            for (size_t j = i + 1; j < route.size(); ++j) {
                const data::StopRecord& current_stop_ptr = route[j - 1];
                const data::StopRecord& next_stop_ptr = route[j];
                if (from_stop_ptr == next_stop_ptr) {  //! Сомнительно!)
                    continue;
                }

                auto it = db_reader_.GetDistanceBetweenStops(current_stop_ptr, next_stop_ptr);
                total_travel_time += it.measured_distance / 1000.0 / settings_.bus_velocity_kmh * 60.0 +
                                     (!bus.is_roundtrip && last_stop_of_route == current_stop_ptr ? settings_.bus_wait_time_min : 0.);

                auto ege_id = graph_.AddEdge({index_mapper_.GetAt(from_stop_ptr), index_mapper_.GetAt(next_stop_ptr), total_travel_time});

                const RoutingItemInfo info{
                    .bus_name = bus.name,
                    .bus_wait_time_min = settings_.bus_wait_time_min,
                    .bus_travel_time = total_travel_time - settings_.bus_wait_time_min,
                    .travel_items_count = span,  // Number of distances between stops (aka span count)
                    .from_stop = from_stop_ptr->name,
                    .next_stop = next_stop_ptr->name,
                    .current_stop = current_stop_ptr->name};
                edges_.emplace(std::move(ege_id), std::move(info));
                ++span;
            }
        }
    }
}
