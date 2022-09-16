#include "map_renderer.h"

#include <algorithm>
#include <cassert>

#include "svg.h"

namespace transport_catalogue::maps /* MapLayer implementation */ {
    svg::Document& MapLayer::GetSvgDocument() {
        return svg_document_;
    }

    svg::Document&& MapLayer::ExtractSvgDocument() {
        return std::move(svg_document_);
    }

    void MapLayer::Draw() {
        svg_document_.Clear();
        std::for_each(objects_.begin(), objects_.end(), [this](auto& obj) {
            obj->Darw(svg_document_);
        });
    }

    MapLayer::ObjectCollection& MapLayer::GetObjects() {
        return objects_;
    }
}

namespace transport_catalogue::maps /* MapRenderer implementation */ {
    void MapRenderer::UpdateLayers() {
        layers_.routes.Draw();
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
        drawable_bus.SetColor(Color(color_picker_.GetRouteColor()));

        layers_.route_names.Add(drawable_bus.BuildLable());
        layers_.routes.Add(std::move(drawable_bus));
        color_picker_.SetNextRouteColor();
    }

    void MapRenderer::AddStopToLayer(const data::StopRecord&& stop_record) {
        assert(!settings_.color_palette.empty());
        StopMarker drawable_stop{stop_record, settings_, projection_};
        drawable_stop.SetColor(Color(color_picker_.GetStopColor()));

        layers_.stop_marker_names.Add(drawable_stop.BuildLable());
        layers_.stop_markers.Add(std::move(drawable_stop));
        color_picker_.SetNextBusColor();
    }

    void MapRenderer::SetRenderSettings(RenderSettings&& settings) {
        settings_ = std::move(settings);
        color_picker_.SetColorPalette(settings_.color_palette);
    }

    RenderSettings& MapRenderer::GetRenderSettings() {
        return settings_;
    }

    MapRenderer::RawMapData MapRenderer::GetRawMap() {
        layers_.DrawMap();
        return layers_.ExtractRawMapData();
    }

    svg::Document& MapRenderer::GetRouteLayer() {
        layers_.routes.Draw();
        return layers_.routes.GetSvgDocument();
    }

    svg::Document& MapRenderer::GetRouteNamesLayer() {
        layers_.route_names.Draw();
        return layers_.route_names.GetSvgDocument();
    }

    svg::Document& MapRenderer::GetStopMarkersLayer() {
        layers_.stop_markers.Draw();
        return layers_.stop_markers.GetSvgDocument();
    }

    svg::Document& MapRenderer::GetStopMarkerNamesLayer() {
        layers_.stop_marker_names.Draw();
        return layers_.stop_marker_names.GetSvgDocument();
    }
}

namespace transport_catalogue::maps /* MapRenderer::BusRoute implementation */ {

    void MapRenderer::BusRoute::UpdateLocation() {
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

    void MapRenderer::BusRoute::Update() {
        IDbObject::Update();
        UpdateLocation();
    }

    std::shared_ptr<IDrawable> MapRenderer::BusRoute::Clone() const {
        return std::make_shared<MapRenderer::BusRoute>(*this);
    }

    void MapRenderer::BusRoute::Build() {
        assert(locations_.empty());

        const data::Route& route = db_record_->route;
        locations_.reserve(route.size());
        std::for_each(route.begin(), route.end(), [this](const data::StopRecord& stop) {
            locations_.emplace_back(projection_.FromLatLngToMapPoint(stop->coordinates), stop->coordinates);
        });
    }
}

namespace transport_catalogue::maps /* MapRenderer::BusRoute::BusRouteLable implementation */ {
    void MapRenderer::BusRoute::BusRouteLable::Darw(svg::ObjectContainer& layer) const {
        assert(HasValidParent());
        if (name_lables_.empty()) {
            return;
        }
        static const std::string font_weight("bold");
        static const std::string font_family("Verdana");
        std::for_each(name_lables_.begin(), name_lables_.end(), [this, &layer](const NameLable& lable) {
            svg::Text base;
            base.SetData(lable.text)
                .SetPosition(lable.location.GetMapPoint())
                .SetOffset(MapPoint(settings_.bus_label_offset))
                .SetFontSize(settings_.bus_label_font_size)
                .SetFontFamily(font_family)
                .SetFontWeight(font_weight);
            svg::Text name = base.SetFillColor(color_);

            svg::Text underlay = base.SetFillColor(settings_.underlayer_color)
                                     .SetStrokeColor(settings_.underlayer_color)
                                     .SetStrokeWidth(settings_.underlayer_width)
                                     .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                                     .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            layer.Add(std::move(underlay));
            layer.Add(std::move(name));
        });
    }

    void MapRenderer::BusRoute::BusRouteLable::Update() {
        assert(HasValidParent());
        IDbObject::Update();
        UpdateLocation();
    }

    void MapRenderer::BusRoute::BusRouteLable::UpdateLocation() {
        Build();
    }

    bool MapRenderer::BusRoute::BusRouteLable::HasValidParent() const {
        return !parent_ref_handle_.expired();
    }

    std::shared_ptr<IDrawable> MapRenderer::BusRoute::BusRouteLable::Clone() const {
        return std::make_shared<MapRenderer::BusRoute::BusRouteLable>(*this);
    }

    void MapRenderer::BusRoute::BusRouteLable::Build() {
        assert(HasValidParent());
        assert(db_record_ == drawable_bus_.db_record_);

        const auto& locations = drawable_bus_.locations_;
        assert(db_record_->route.size() == drawable_bus_.locations_.size());

        if (locations.empty()) {
            return;
        }

        const std::string& name = db_record_->name;
        name_lables_.emplace_back(name, locations.front());
        
        //!! Need edit for mig to db_record_->GetLastStopOfRoute()
        if (locations.size() > 1 && !db_record_->is_roundtrip) {
            auto center = static_cast<size_t>(locations.size() / 2ul);
            if (db_record_->route[center]->name != db_record_->route.front()->name) {
                name_lables_.emplace_back(name, locations[center]);
            }
        }
    }
}

namespace transport_catalogue::maps /* MapRenderer::StopMarker implementation */ {
    void MapRenderer::StopMarker::Update() {
        IDbObject::Update();
        UpdateLocation();
    }

    void MapRenderer::StopMarker::UpdateLocation() {
        location_.GetMapPoint() = {projection_.FromLatLngToMapPoint(location_.GetGlobalCoordinates())};
    }

    void MapRenderer::StopMarker::Darw(svg::ObjectContainer& layer) const {
        static const std::string color{"white"};
        layer.Add(svg::Circle().SetCenter(location_.GetMapPoint()).SetFillColor(color).SetRadius(settings_.stop_marker_radius));
    }

    void MapRenderer::StopMarker::SetColor(const Color&& color) {
        color_ = std::move(color);
    }

    MapRenderer::StopMarker::StopMarkerLable MapRenderer::StopMarker::BuildLable() const {
        return StopMarkerLable(*this);
    }

    std::shared_ptr<IDrawable> MapRenderer::StopMarker::Clone() const {
        return std::make_shared<MapRenderer::StopMarker>(*this);
    }

    const Location& MapRenderer::StopMarker::GetLocation() const {
        return location_;
    }

    const std::string& MapRenderer::StopMarker::GetName() const {
        return db_record_->name;
    }

    void MapRenderer::StopMarker::Build() {
        location_ = Location(projection_.FromLatLngToMapPoint(db_record_->coordinates), db_record_->coordinates);
    }
}

namespace transport_catalogue::maps /* MapRenderer::StopMarker::StopMarkerLable implementation */ {

    void MapRenderer::StopMarker::StopMarkerLable::Darw(svg::ObjectContainer& layer) const {
        assert(HasValidParent());
        if (name_lables_.empty()) {
            return;
        }

        static const std::string default_color("black");
        static const std::string default_font_family("Verdana");
        std::for_each(name_lables_.begin(), name_lables_.end(), [this, &layer](const NameLable& lable) {
            svg::Text base;
            base.SetData(lable.text)
                .SetPosition(lable.location.GetMapPoint())
                .SetOffset(MapPoint(settings_.stop_label_offset))
                .SetFontSize(settings_.stop_label_font_size)
                .SetFontFamily(default_font_family);

            svg::Text name = base.SetFillColor(default_color);

            svg::Text underlay = base.SetFillColor(settings_.underlayer_color)
                                     .SetStrokeColor(settings_.underlayer_color)
                                     .SetStrokeWidth(settings_.underlayer_width)
                                     .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                                     .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            layer.Add(std::move(underlay));
            layer.Add(std::move(name));
        });
    }

    void MapRenderer::StopMarker::StopMarkerLable::Update() {
        assert(HasValidParent());
        IDbObject::Update();
        UpdateLocation();
    }

    void MapRenderer::StopMarker::StopMarkerLable::UpdateLocation() {
        Build();
    }

    bool MapRenderer::StopMarker::StopMarkerLable::HasValidParent() const {
        return !parent_ref_handle_.expired();
    }

    std::shared_ptr<IDrawable> MapRenderer::StopMarker::StopMarkerLable::Clone() const {
        return std::make_shared<MapRenderer::StopMarker::StopMarkerLable>(*this);
    }

    void MapRenderer::StopMarker::StopMarkerLable::Build() {
        assert(HasValidParent());
        assert(db_record_ == bus_marker_.db_record_);

        name_lables_.emplace_back(db_record_->name, bus_marker_.location_);
    }
}

namespace transport_catalogue::maps /* ColorPaletteСyclicIterator implementation */ {

    ColorPalette::const_iterator ColorPaletteСyclicIterator::NextColor() {
        if (curr_it_ != palette_.end() && ++curr_it_ != palette_.end()) {
            return curr_it_;
        } else {
            curr_it_ = palette_.begin();
            return curr_it_;
        }
    }

    const ColorPalette& ColorPaletteСyclicIterator::GetColorPalette() const {
        return palette_;
    }

    const ColorPalette::value_type& ColorPaletteСyclicIterator::GetCurrentColor() const {
        assert(!IsEmpty());
        return *curr_it_;
    }

    const ColorPalette::value_type& ColorPaletteСyclicIterator::GetPrevColor() const {
        assert(!IsEmpty());
        auto prev_it = curr_it_;
        if (prev_it != palette_.begin() && --prev_it != palette_.begin()) {
            return *prev_it;
        } else {
            return *std::prev(palette_.end());
        }
    }

    const ColorPalette::value_type& ColorPaletteСyclicIterator::GetCurrentColorOrNone() const noexcept {
        return IsEmpty() ? NoneColor : GetCurrentColor();
    }

    const ColorPalette::value_type& ColorPaletteСyclicIterator::GetPrevColorOrNone() const noexcept {
        return IsEmpty() ? NoneColor : GetPrevColor();
    }

    bool ColorPaletteСyclicIterator::IsEmpty() const {
        return palette_.empty();
    }

    void ColorPaletteСyclicIterator::SetColorPalette(ColorPalette&& new_palette) {
        palette_ = std::move(new_palette);
        curr_it_ = palette_.begin();
    }
}

namespace transport_catalogue::maps /* MapRenderer::ColorPicker implementation */ {

    Color MapRenderer::ColorPicker::GetRouteColor() {
        return route_colors_iterator_.GetCurrentColorOrNone();
    }
    Color MapRenderer::ColorPicker::GetStopColor() {
        return busses_colors_iterator_.GetCurrentColorOrNone();
    }

    void MapRenderer::ColorPicker::SetNextRouteColor() {
        route_colors_iterator_.NextColor();
    }
    void MapRenderer::ColorPicker::SetNextBusColor() {
        busses_colors_iterator_.NextColor();
    }

    void MapRenderer::ColorPicker::SetColorPalette(ColorPalette colors) {
        route_colors_iterator_.SetColorPalette(ColorPalette{colors});
        busses_colors_iterator_.SetColorPalette(ColorPalette{colors});
    }
}

namespace transport_catalogue::maps /* MapRenderer::LayerSet implementation */ {
    void MapRenderer::LayerSet::DrawMap() {
        routes.Draw();
        route_names.Draw();
        stop_markers.Draw();
        stop_marker_names.Draw();
    }

    MapRenderer::RawMapData MapRenderer::LayerSet::ExtractRawMapData() {
        svg::Document doc;
        doc.MoveObjectsFrom(routes.ExtractSvgDocument());
        doc.MoveObjectsFrom(route_names.ExtractSvgDocument());
        doc.MoveObjectsFrom(stop_markers.ExtractSvgDocument());
        doc.MoveObjectsFrom(stop_marker_names.ExtractSvgDocument());
        std::ostringstream stream;
        doc.Render(stream);
        return stream.str();
    }
}

namespace transport_catalogue::maps /* Location implementation */ {
    
    void Location::Transform(const geo::Projection& projection) {
        const auto [north, east] = projection.FromLatLngToMapPoint(lat_lng_);
        map_position_.north = north;
        map_position_.east = east;
    }

    const MapPoint& Location::GetMapPoint() const {
        return map_position_;
    }

    MapPoint& Location::GetMapPoint() {
        return map_position_;
    }

    const GlobalCoordinates& Location::GetGlobalCoordinates() const {
        return lat_lng_;
    }

    GlobalCoordinates& Location::GetGlobalCoordinates() {
        return lat_lng_;
    }
}