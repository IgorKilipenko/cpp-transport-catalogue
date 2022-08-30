/*
 * В этом файле вы можете разместить код, отвечающий за визуализацию карты маршрутов в формате SVG.
 * Визуализация маршрутов вам понадобится во второй части итогового проекта.
 * Пока можете оставить файл пустым.
 */

#include "map_renderer.h"

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
        drawable_bus.SetColor(Color(color_palette_iterator_.GetCurrentColor()));
        drawable_bus.Darw(routes_layer_.GetSvgDocument());
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
        return GetTransportLayerMap();
    }

    svg::Document& MapRenderer::GetTransportLayerMap() {
        routes_layer_.Draw();
        return routes_layer_.GetSvgDocument();
    }
}