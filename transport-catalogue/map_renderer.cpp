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
        assert(!settings_.color_palette.empty());
        BusRoute drawable_bus{bus_record, settings_, projection_};
        drawable_bus.SetColor(Color(route_color_palette_iterator_.GetCurrentColor()));

        route_names_layer_.Add(drawable_bus.BuildLable());
        routes_layer_.Add(std::move(drawable_bus));
        route_color_palette_iterator_.NextColor();
    }

    void MapRenderer::AddStopToLayer(const data::StopRecord&& stop_record) {
        assert(!settings_.color_palette.empty());
        StopMarker drawable_stop{stop_record, settings_, projection_};
        drawable_stop.SetColor(Color(busses_color_palette_iterator_.GetCurrentColor()));

        // stop_marker_names_layer_.Add(drawable_stop.BuildLable());
        stop_markers_layer_.Add(std::move(drawable_stop));
        busses_color_palette_iterator_.NextColor();
    }

    void MapRenderer::SetRenderSettings(RenderSettings&& settings) {
        settings_ = std::move(settings);
        route_color_palette_iterator_.SetColorPalette(ColorPalette(settings_.color_palette));
        busses_color_palette_iterator_.SetColorPalette(ColorPalette(settings_.color_palette));
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
        route_names_layer_.Draw();
        return route_names_layer_.GetSvgDocument();
    }

    svg::Document& MapRenderer::GetStopMarkersLayer() {
        stop_markers_layer_.Draw();
        return stop_markers_layer_.GetSvgDocument();
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

namespace transport_catalogue::maps /* MapRenderer::StopMarker implementation */ {
    void MapRenderer::StopMarker::Update()  {
        IDbObject::Update();
        UpdateLocation();
    }

    void MapRenderer::StopMarker::UpdateLocation()  {
        location_.GetMapPoint() = {projection_.FromLatLngToMapPoint(location_.GetGlobalCoordinates())};
    }

    void MapRenderer::StopMarker::Darw(svg::ObjectContainer& layer) const  {
        static const std::string color{"white"};
        layer.Add(svg::Circle().SetCenter(location_.GetMapPoint()).SetFillColor(color).SetRadius(settings_.stop_marker_radius));
    }

    void MapRenderer::StopMarker::SetColor(const Color&& color) {
        color_ = std::move(color);
    }

    MapRenderer::StopMarker::StopMarkerLable MapRenderer::StopMarker::BuildLable() const {
        throw std::runtime_error("Not implemented");
    }

    std::shared_ptr<IDrawable> MapRenderer::StopMarker::Clone() const  {
        return std::make_shared<MapRenderer::StopMarker>(*this);
    }
}