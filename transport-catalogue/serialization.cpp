#include "serialization.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "domain.h"
#include "graph.pb.h"
#include "map_renderer.h"
#include "svg.h"
#include "transport_catalogue.pb.h"
#include "transport_router.h"
#include "transport_router.pb.h"

namespace transport_catalogue::serialization /* DataConvertor implementation */ {
    template <>
    auto DataConverter::ConvertToModel(data::StopRecord stop) const {
        StopModel stop_model;
        stop_model.set_name(stop->name);

        CoordinatesModel coordinates;
        coordinates.set_lat(stop->coordinates.lat);
        coordinates.set_lng(stop->coordinates.lng);

        *stop_model.mutable_coordinates() = std::move(coordinates);

        return stop_model;
    }

    template <>
    auto DataConverter::ConvertToModel(data::BusRecord bus) const {
        BusModel bus_model;
        bus_model.set_name(bus->name);
        bus_model.set_is_roundtrip(bus->is_roundtrip);
        std::for_each(bus->route.begin(), bus->route.end(), [&](data::StopRecord stop) {
            bus_model.add_route(stop->name);
        });

        return bus_model;
    }

    template <>
    auto DataConverter::ConvertToModel(const data::Bus& bus) const {
        data::BusRecord bus_record = &bus;
        return ConvertToModel(bus_record);
    }

    template <>
    auto DataConverter::ConvertToModel(const data::Stop& stop) const {
        data::StopRecord stop_record = &stop;
        return ConvertToModel(stop_record);
    }

    template <>
    auto DataConverter::ConvertToModel(DistanceBetweenStopsItem&& distance_item) const {
        DistancesBetweenStopsModel distance_item_model;
        distance_item_model.set_from_stop(distance_item.from_stop->name);
        distance_item_model.set_to_stop(distance_item.to_stop->name);
        distance_item_model.set_distance(distance_item.distance_between);
        return distance_item_model;
    }

    template <>
    auto DataConverter::ConvertToModel(const maps::Offset& offset) const {
        proto_schema::maps::Offset offset_model;
        offset_model.set_east(offset.east);
        offset_model.set_north(offset.north);

        return offset_model;
    }

    template <>
    auto DataConverter::ConvertToModel(const svg::Rgb& rgb) const {
        proto_schema::svg::Rgb color_rgb_model;
        color_rgb_model.set_red(rgb.red);
        color_rgb_model.set_green(rgb.green);
        color_rgb_model.set_blue(rgb.blue);

        return color_rgb_model;
    }

    template <>
    auto DataConverter::ConvertToModel(svg::Rgb&& rgb) const {
        return ConvertToModel<const svg::Rgb&>(rgb);
    }

    template <>
    auto DataConverter::ConvertToModel(const svg::Rgba& rgba) const {
        proto_schema::svg::Rgba rgba_model;
        *rgba_model.mutable_rgb() = ConvertToModel(svg::Rgb(rgba));
        rgba_model.set_alpha(rgba.opacity);

        return rgba_model;
    }

    template <>
    auto DataConverter::ConvertToModel(const svg::Color& color_svg) const {
        ColorModel color_model;
        if (const svg::Rgb* color = std::get_if<svg::Rgb>(&color_svg); color != nullptr) {
            *color_model.mutable_rgb() = ConvertToModel(*color);
        } else if (const svg::Rgba* color = std::get_if<svg::Rgba>(&color_svg); color != nullptr) {
            *color_model.mutable_rgba() = ConvertToModel(*color);
        } else if (const std::string* color = std::get_if<std::string>(&color_svg); color != nullptr) {
            color_model.set_named(*color);
        }
        return color_model;
    }

    template <>
    auto DataConverter::ConvertFromModel(ColorModel&& color) const {
        svg::Color result;
        if (color.has_rgb()) {
            result = svg::Rgb(color.rgb().red(), color.rgb().green(), color.rgb().blue());
            return result;
        } else if (color.has_rgba()) {
            result = svg::Rgba(color.rgba().rgb().red(), color.rgba().rgb().green(), color.rgba().rgb().blue(), color.rgba().alpha());
            return result;
        }

        result = color.named();
        return result;
    }

    template <>
    auto DataConverter::ConvertToModel(const maps::RenderSettings& settings) const {
        proto_schema::maps::RenderSettings settings_model;
        auto map_box = [&settings] {
            proto_schema::maps::Container map_box;
            map_box.set_height(settings.map_size.height);
            map_box.set_width(settings.map_size.width);
            map_box.set_padding(settings.padding);
            return map_box;
        }();

        *settings_model.mutable_container() = map_box;

        auto decoration = [this, &settings]() {
            proto_schema::maps::Decoration decoration;
            decoration.set_line_width(settings.line_width);
            decoration.set_stop_marker_radius(settings.stop_marker_radius);
            decoration.set_underlayer_width(settings.underlayer_width);
            *decoration.mutable_underlayer_color() = ConvertToModel(settings.underlayer_color);
            return decoration;
        }();

        *settings_model.mutable_decoration() = decoration;

        auto bus_label = [this, &settings]() {
            proto_schema::maps::Label bus_label;
            bus_label.set_font_size(settings.bus_label_font_size);
            *bus_label.mutable_offset() = ConvertToModel(settings.bus_label_offset);
            return bus_label;
        }();

        *settings_model.mutable_bus_label() = bus_label;

        auto stop_label = [this, &settings]() {
            proto_schema::maps::Label stop_label;
            stop_label.set_font_size(settings.stop_label_font_size);
            *stop_label.mutable_offset() = ConvertToModel(settings.stop_label_offset);
            return stop_label;
        }();

        *settings_model.mutable_stop_label() = stop_label;

        proto_schema::maps::ColorPalette* color_palette_model = settings_model.mutable_color_palette();
        std::for_each(settings.color_palette.begin(), settings.color_palette.end(), [&](const maps::Color& color) {
            auto ff = ConvertToModel(color);
            *color_palette_model->add_color_palette() = ConvertToModel(color);
        });

        return settings_model;
    }

    template <>
    auto DataConverter::ConvertFromModel(RenderSettingsModel&& settings_model) const {
        maps::RenderSettings settings;

        proto_schema::maps::Container container_model = std::move(*settings_model.mutable_container());
        const maps::Size map_size(container_model.height(), container_model.width());
        settings.map_size = std::move(map_size);
        settings.padding = container_model.padding();

        settings.bus_label_font_size = settings_model.bus_label().font_size();
        settings.bus_label_offset = maps::Offset(settings_model.bus_label().offset().north(), settings_model.bus_label().offset().east());

        settings.stop_label_font_size = settings_model.stop_label().font_size();
        settings.stop_label_offset = maps::Offset(settings_model.stop_label().offset().north(), settings_model.stop_label().offset().east());

        proto_schema::maps::Decoration decoration_model = std::move(*settings_model.mutable_decoration());
        settings.line_width = decoration_model.line_width();
        settings.underlayer_width = decoration_model.underlayer_width();
        settings.stop_marker_radius = decoration_model.stop_marker_radius();
        settings.underlayer_color = ConvertFromModel(std::move(*decoration_model.mutable_underlayer_color()));

        auto color_palette_model = std::move(*settings_model.mutable_color_palette()->mutable_color_palette());
        maps::ColorPalette color_palette(color_palette_model.size());
        std::transform(
            std::move_iterator(color_palette_model.begin()), std::move_iterator(color_palette_model.end()), color_palette.begin(),
            [&](auto&& color_model) {
                return ConvertFromModel(std::move(color_model));
            });

        settings.color_palette = std::move(color_palette);

        return settings;
    }

    template <>
    auto DataConverter::ConvertToModel(const router::RoutingSettings& settings) const {
        RoutingSettingsModel settings_model;
        settings_model.set_bus_velocity_kmh(settings.bus_velocity_kmh);
        settings_model.set_bus_wait_time_min(settings.bus_wait_time_min);
        return settings_model;
    }

    template <>
    auto DataConverter::ConvertFromModel(RoutingSettingsModel&& settings_model) const {
        router::RoutingSettings settings;
        settings.bus_velocity_kmh = settings_model.bus_velocity_kmh();
        settings.bus_wait_time_min = settings_model.bus_wait_time_min();
        return settings;
    }

    template <>
    auto DataConverter::ConvertToModel(const router::RoutingGraph& graph) const {
        RoutingGraphModel graph_model;
        for (size_t i = 0; i < graph.GetEdgeCount(); ++i) {
            EdgeModel edge_model;
            const graph::Edge<double>& edge = graph.GetEdge(i);
            edge_model.set_from(edge.from);
            edge_model.set_to(edge.to);
            edge_model.set_weight(edge.weight);
            *graph_model.add_edges() = edge_model;
        }
        for (size_t i = 0; i < graph.GetVertexCount(); ++i) {
            IncidentEdgesModel incident_edges_model;
            auto incident_edges = graph.GetIncidentEdges(i);
            std::for_each(incident_edges.begin(), incident_edges.end(), [&incident_edges_model](graph::EdgeId edge_id) {
                incident_edges_model.add_edge_id(static_cast<uint32_t>(edge_id));
            });
            *graph_model.add_incident_edges() = std::move(incident_edges_model);
        }
        return graph_model;
    }

    template <>
    auto DataConverter::ConvertFromModel(RoutingGraphModel&& graph_model) const {
        auto edges_model = std::move(*graph_model.mutable_edges());
        std::vector<router::RoutingGraph::EdgeType> edges;
        edges.reserve(edges_model.size());
        for (int i = 0; i < edges_model.size(); ++i) {
            EdgeModel edge_model = std::move(edges_model[i]);

            router::RoutingGraph::EdgeType edge;
            edge.from = edge_model.from();
            edge.to = edge_model.to();
            edge.weight = edge_model.weight();
            edges.emplace_back(std::move(edge));
        }

        router::RoutingGraph::IncidentEdges incident_edges;
        incident_edges.reserve(graph_model.incident_edges_size());
        for (int i = 0; i < graph_model.incident_edges_size(); ++i) {
            IncidentEdgesModel incidence_list_model = graph_model.incident_edges(i);
            router::RoutingGraph::IncidenceList incidence_list;
            incidence_list.reserve(incidence_list_model.edge_id_size());
            for (int j = 0; j < incidence_list_model.edge_id_size(); ++j) {
                incidence_list.push_back(incidence_list_model.edge_id(j));
            }
            incident_edges.push_back(std::move(incidence_list));
        }

        return router::RoutingGraph(std::move(edges), std::move(incident_edges));
    }

    template <>
    auto DataConverter::ConvertToModel(const router::RoutingItemInfo& route_item) const {
        proto_schema::router::RoutingItemInfo route_item_model;
        route_item_model.set_bus_name(std::string(route_item.bus_name));
        route_item_model.set_stop_name(std::string(route_item.stop_name));
        route_item_model.set_travel_items_count(route_item.travel_items_count);
        route_item_model.set_bus_travel_time(route_item.bus_travel_time);
        route_item_model.set_bus_wait_time_min(route_item.bus_wait_time_min);

        return route_item_model;
    }

    template <>
    auto DataConverter::ConvertFromModel(RoutingItemModel&& route_item_model, const data::ITransportDataReader& db_reader) const {
        router::RoutingItemInfo route_item;
        route_item.bus_name = db_reader.GetBus(route_item_model.bus_name())->name;
        route_item.stop_name = db_reader.GetStop(route_item_model.stop_name())->name;
        route_item.travel_items_count = route_item_model.travel_items_count();
        route_item.bus_travel_time = route_item_model.bus_travel_time();
        route_item.bus_wait_time_min = route_item_model.bus_wait_time_min();

        return route_item;
    }

    template <>
    auto DataConverter::ConvertFromModel(RoutingItemsModel&& route_items_model, const data::ITransportDataReader& db_reader) const {
        router::RoutingIncidentEdges route_items;
        for (int i = 0; i < route_items_model.size(); ++i) {
            route_items.emplace(i, ConvertFromModel<RoutingItemModel, const data::ITransportDataReader&>(std::move(route_items_model[i]), db_reader));
        }
        return route_items;
    }
}

namespace transport_catalogue::serialization /* Store (serialize) implementation */ {

    void Store::PrepareBuses(TransportDataModel& container) const {
        const data::DatabaseScheme::BusRoutesTable& buses = db_reader_.GetDataReader().GetBusRoutesTable();
        std::for_each(buses.begin(), buses.end(), [&](const data::Bus& bus) {
            *container.add_buses() = converter_.ConvertToModel(bus);
        });
    }

    void Store::PrepareStops(TransportDataModel& container) const {
        const data::DatabaseScheme::StopsTable& stops = db_reader_.GetDataReader().GetStopsTable();
        std::for_each(stops.begin(), stops.end(), [&](const data::Stop& stop) {
            *container.add_stops() = converter_.ConvertToModel(stop);
        });
    }

    void Store::PrepareDistances(TransportDataModel& container) const {
        const data::DatabaseScheme::DistanceBetweenStopsTable& distances = db_reader_.GetDataReader().GetDistancesBetweenStops();
        std::for_each(distances.begin(), distances.end(), [&](const data::DatabaseScheme::DistanceBetweenStopsTable::value_type& dist_item) {
            *container.add_distances() = converter_.ConvertToModel(
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

    void Store::PrepareRenderSettings(SettingsModel& settings) const {
        *settings.mutable_render_settings() = converter_.ConvertToModel(map_renderer_.GetRenderSettings());
    }

    void Store::PrepareRoutingSettings(SettingsModel& settings) const {
        *settings.mutable_routing_settings() = converter_.ConvertToModel(transport_router_.GetSettings());
    }

    SettingsModel Store::BuildSerializableSettings() const {
        SettingsModel settings_model;
        PrepareRenderSettings(settings_model);
        PrepareRoutingSettings(settings_model);
        return settings_model;
    }

    void Store::PrepareGraphModel(RouterModel& router_model) const {
        *router_model.mutable_graph() = converter_.ConvertToModel(transport_router_.GetGraph());
    }

    void Store::PrepareRouterModel(RouterModel& router_model) const {
        for (size_t i = 0; i < transport_router_.GetGraph().GetEdgeCount(); ++i) {  //! need edit
            const router::RoutingItemInfo& route_item = transport_router_.GetRoutingItem(i);
            proto_schema::router::RoutingItemInfo route_item_model = converter_.ConvertToModel(route_item);
            *router_model.add_routing_items() = std::move(route_item_model);
        }
    }

    RouterModel Store::BuildSerializableRouterModel() const {
        RouterModel router_model;
        PrepareGraphModel(router_model);
        PrepareRouterModel(router_model);

        return router_model;
    }

    bool Store::SaveToStorage() {
        if (!db_path_.has_value()) {
            return false;
        }

        std::ofstream out(db_path_.value(), std::ios::binary);

        DatabaseModel database_model;
        *database_model.mutable_transport_data() = BuildSerializableTransportData();
        *database_model.mutable_settings() = BuildSerializableSettings();
        *database_model.mutable_router() = BuildSerializableRouterModel();

        const bool success = database_model.SerializeToOstream(&out);
        assert(success);

        return success;
    }
}

namespace transport_catalogue::serialization /* Store (deserialize) implementation */ {

    void Store::FillTransportData(TransportDataModel&& data) const {
        auto stops = std::move(*data.mutable_stops());
        std::for_each(std::move_iterator(stops.begin()), std::move_iterator(stops.end()), [&](StopModel&& stop) {
            db_writer_.AddStop(std::move(*stop.mutable_name()), {stop.coordinates().lat(), stop.coordinates().lng()});
        });

        auto distances = std::move(*data.mutable_distances());
        std::for_each(std::move_iterator(distances.begin()), std::move_iterator(distances.end()), [&](DistancesBetweenStopsModel&& dist_item) {
            db_writer_.SetMeasuredDistance(
                data::MeasuredRoadDistance(std::move(*dist_item.mutable_from_stop()), std::move(*dist_item.mutable_to_stop()), dist_item.distance()));
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
    }

    void Store::FillRenderSettings(RenderSettingsModel&& render_settings_model) const {
        map_renderer_.SetRenderSettings(converter_.ConvertFromModel(std::move(render_settings_model)));
    }

    void Store::FillRoutingSettings(RoutingSettingsModel&& routing_settings_model) const {
        transport_router_.SetSettings(converter_.ConvertFromModel(std::move(routing_settings_model)));
    }

    void Store::FillSettings(SettingsModel&& settings_model) const {
        RenderSettingsModel render_settings_model = std::move(*settings_model.mutable_render_settings());
        FillRenderSettings(std::move(render_settings_model));

        RoutingSettingsModel routing_settings_model = std::move(*settings_model.mutable_routing_settings());
        FillRoutingSettings(std::move(routing_settings_model));
    }

    void Store::FillRouter(RouterModel&& router_model) const {
        RoutingGraphModel graph_model = std::move(*router_model.mutable_graph());
        router::RoutingGraph graph = converter_.ConvertFromModel(std::move(graph_model));
        transport_router_.SetGraph(
            std::move(graph), converter_.ConvertFromModel<RoutingItemsModel, const data::ITransportDataReader&>(
                                  std::move(*router_model.mutable_routing_items()), db_reader_.GetDataReader()));
    }

    bool Store::LoadDatabase() const {
        if (!db_path_.has_value()) {
            return false;
        }

        DatabaseModel db_model;

        std::ifstream in(db_path_.value(), std::ios::binary);
        assert(db_model.ParseFromIstream(&in));

        FillTransportData(std::move(*db_model.mutable_transport_data()));
        FillSettings(std::move(*db_model.mutable_settings()));
        FillRouter(std::move(*db_model.mutable_router()));

        return true;
    }
}