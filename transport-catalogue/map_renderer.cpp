/*
 * В этом файле вы можете разместить код, отвечающий за визуализацию карты маршрутов в формате SVG.
 * Визуализация маршрутов вам понадобится во второй части итогового проекта.
 * Пока можете оставить файл пустым.
 */

#include "map_renderer.h"

#include <algorithm>
#include <cassert>

#include "svg.h"

namespace transport_catalogue::maps /* MapRenderer implementation */ {
    void MapRenderer::UpdateLayers() {
        routes_layer_.Draw();
    }

    void MapRenderer::UpdateMapProjection(Projection_&& projection) {
        if (projection == projection_) {
            return;
        }
        projection_ = std::move(projection);
        UpdateLayers();
    }

    void MapRenderer::AddRouteToLayer(const data::BusRecord&& bus_record) {
        /*
        assert(!settings_.color_palette.empty());
        BusRoute drawable_bus{bus_record, settings_, projection_};
        drawable_bus.SetColor(Color(color_palette_iterator_.GetCurrentColor()));
        drawable_bus.Darw(routes_layer_.GetSvgDocument());
        color_palette_iterator_.NextColor();
        */
        assert(!settings_.color_palette.empty());
        BusRoute drawable_bus{bus_record, settings_, projection_};
        drawable_bus.SetColor(Color(color_palette_iterator_.GetCurrentColor()));
        
        route_names_layer_.Add(drawable_bus.BuildLable()); 
        routes_layer_.Add(std::move(drawable_bus));
        color_palette_iterator_.NextColor();
    }

    void MapRenderer::AddRouteNameToLayer(data::BusRecord bus_record) {}

    void MapRenderer::DrawTransportTracksLayer(std::vector<data::BusRecord>&& /*records*/) {
        throw std::runtime_error("Not implemented");  //! Not implemented
    }

    void MapRenderer::DrawTransportStopsLayer(std::vector<data::StopRecord>&& /*records*/) {
        throw std::runtime_error("Not implemented");  //! Not implemented
    }

    void MapRenderer::SetRenderSettings(RenderSettings&& settings) {
        settings_ = std::move(settings);
        color_palette_iterator_.SetColorPalette(ColorPalette(settings_.color_palette));
        //! UpdateLayers();
    }

    RenderSettings& MapRenderer::GetRenderSettings() {
        return settings_;
    }

    svg::Document& MapRenderer::GetMap() {
        //! UpdateLayers();
        return GetRouteLayer();
    }

    svg::Document& MapRenderer::GetRouteLayer() {       
        routes_layer_.Draw();
        return routes_layer_.GetSvgDocument();
    }

    svg::Document& MapRenderer::GetRouteNamesLayer() {
        // routes_layer_.Draw();
        auto& routes = routes_layer_.GetObjects();
        auto& route_names = route_names_layer_.GetObjects();
        assert(routes.size() == route_names.size());

        std::vector<IDrawable*> objects;
        // objects.resize(routes.size() + route_names.size());
        /*for (auto route_it = routes.begin(), name_it = route_names.begin(); route_it != routes.end(); ++route_it, ++name_it) {
            objects.emplace_back((*route_it).)
        }*/

        // std::copy(routes.begin(), routes.end(), std::back_inserter(objects));
        // std::copy(route_names.begin(), route_names.end(), std::back_inserter(objects));
        svg::Document& map = map_layer_.GetSvgDocument();
        std::for_each(routes.begin(), routes.end(), [&map](auto& route) {
            route->Darw(map);
        });
        return map;
        // return routes_layer_.GetSvgDocument();
    }
}

namespace transport_catalogue::maps /* MapRenderer::BusRoute implementation */ {

    void MapRenderer::BusRoute::UpdateLocation() {
        // std::cerr << "BusRoute -> Update Location" << std::endl;  //! FOR DEBUG ONLY
        std::for_each(locations_.begin(), locations_.end(), [this](Location& location) {
            location.GetMapPoint() = {projection_.FromLatLngToMapPoint(location.GetGlobalCoordinates())};
        });
    }

    void MapRenderer::BusRoute::Darw(svg::ObjectContainer& layer) const {
        layer.Add(svg::Polyline(locations_)
                      .SetFillColor(svg::NoneColor)
                      .SetStrokeWidth(settings_.line_width)
                      .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                      .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
                      .SetStrokeColor(color_));
    }

    const data::Route& MapRenderer::BusRoute::GetRoute() const {
        return db_record_->route;
    }

    void MapRenderer::BusRoute::SetColor(const Color&& color) {
        color_ = std::move(color);
    }

    MapRenderer::BusRoute::BusRouteLable MapRenderer::BusRoute::BuildLable() const {
        return BusRouteLable(*this);
    }
}