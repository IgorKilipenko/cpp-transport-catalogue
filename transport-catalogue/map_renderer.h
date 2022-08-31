/*
 * В этом файле вы можете разместить код, отвечающий за визуализацию карты маршрутов в формате SVG.
 * Визуализация маршрутов вам понадобится во второй части итогового проекта.
 * Пока можете оставить файл пустым.
 */

#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "detail/type_traits.h"
#include "domain.h"
#include "geo.h"
#include "svg.h"
#include "transport_catalogue.h"

namespace transport_catalogue::maps /* Aliases */ {
    using Color = svg::Color;
    using ColorPalette = std::vector<Color>;
    using Offset = geo::Offset;
    using Size = geo::Size;
    using GlobalCoordinates = geo::Coordinates;
}

namespace transport_catalogue::maps /* Locations */ {

    struct MapPoint : public geo::Point {
        using geo::Point::Point;
        MapPoint(geo::Point point) : geo::Point(std::move(point)) {}
        operator svg::Point() const {
            return svg::Point(north, east);
        }
    };

    class Location {
    public:
        Location() : Location({}, {}) {}
        Location(MapPoint map_position, GlobalCoordinates lat_lng) : map_position_{map_position}, lat_lng_{lat_lng} {}

        void Transform(const geo::Projection& projection) {
            const auto [north, east] = projection.FromLatLngToMapPoint(lat_lng_);
            map_position_.north = north;
            map_position_.east = east;
        }

        const MapPoint& GetMapPoint() const {
            return map_position_;
        }

        MapPoint& GetMapPoint() {
            return map_position_;
        }

        const GlobalCoordinates& GetGlobalCoordinates() const {
            return lat_lng_;
        }

        GlobalCoordinates& GetGlobalCoordinates() {
            return lat_lng_;
        }

    private:
        MapPoint map_position_;
        GlobalCoordinates lat_lng_;
    };

    class Polyline : public std::vector<Location> {
    public:
        using std::vector<Location>::vector;
        using ValueType = std::vector<Location>::value_type;

        template <typename Point>
        operator std::vector<Point>() const {
            std::vector<Point> result(size());
            std::transform(begin(), end(), result.begin(), [](const Location& location) {
                return static_cast<Point>(location.GetMapPoint());
            });
            return result;
        }

        template <typename Point>
        std::vector<Point> ExtractPoints() {
            std::vector<Point> result(size());
            std::transform(std::make_move_iterator(begin()), std::make_move_iterator(end()), result.begin(), [](Location&& location) {
                return static_cast<Point>(std::move(location.GetMapPoint()));
            });
            return result;
        }
    };

}

namespace transport_catalogue::maps {

    class ColorPaletteСyclicIterator {
    public:
        inline static const ColorPalette::value_type NoneColor = svg::Colors::NoneColor;

        ColorPaletteСyclicIterator() : ColorPaletteСyclicIterator(ColorPalette()) {}
        ColorPaletteСyclicIterator(ColorPalette&& palette) : palette_(std::move(palette)), curr_it_{palette_.begin()} {}

        ColorPalette::const_iterator NextColor() {
            if (curr_it_ != palette_.end() && ++curr_it_ != palette_.end()) {
                return curr_it_;
            } else {
                curr_it_ = palette_.begin();
                return curr_it_;
            }
        }

        const ColorPalette& GetColorPalette() const {
            return palette_;
        }

        const ColorPalette::value_type& GetCurrentColor() const {
            assert(!IsEmpty());
            return *curr_it_;
        }

        const ColorPalette::value_type& GetPrevColor() const {
            assert(!IsEmpty());
            auto prev_it = curr_it_;
            if (prev_it != palette_.begin() && --prev_it != palette_.begin()) {
                return *prev_it;
            } else {
                return *std::prev(palette_.end());
            }
        }

        const ColorPalette::value_type& GetCurrentColorOrNone() const noexcept {
            return IsEmpty() ? NoneColor : GetCurrentColor();
        }

        const ColorPalette::value_type& GetPrevColorOrNone() const noexcept {
            return IsEmpty() ? NoneColor : GetPrevColor();
        }

        bool IsEmpty() const {
            return palette_.empty();
        }

        void SetColorPalette(ColorPalette&& new_palette) {
            palette_ = std::move(new_palette);
            curr_it_ = palette_.begin();
        }

    private:
        ColorPalette palette_;
        ColorPalette::iterator curr_it_;
    };

    struct RenderSettings {
        Size map_size;

        double padding = 0.0;

        double line_width = 0.0;
        double stop_marker_radius = 0;

        int bus_label_font_size = 0;
        Offset bus_label_offset;

        int stop_label_font_size = 0;
        Offset stop_label_offset;

        Color underlayer_color;
        double underlayer_width = 0.0;

        ColorPalette color_palette;
    };

    class IDrawable {
    public:
        virtual void Darw(svg::ObjectContainer& layer) const = 0;
        virtual std::shared_ptr<IDrawable> Clone() const = 0;
        virtual ~IDrawable() = default;

    protected:
        IDrawable(const RenderSettings& settings) : settings_(settings) {}

    protected:
        const RenderSettings& settings_;
    };
}

namespace transport_catalogue::maps /* MapLayer */ {
    class MapLayer {
    public:
        using ObjectCollection = std::vector<std::shared_ptr<IDrawable>>;
        svg::Document& GetSvgDocument() {
            return svg_document_;
        }

        void Draw() {
            svg_document_.Clear();
            std::for_each(objects_.begin(), objects_.end(), [this](auto& obj) {
                obj->Darw(svg_document_);
            });
        }

        template <typename DrawableType>
        void Add(DrawableType&& obj) {
            objects_.emplace_back(std::make_shared<std::decay_t<DrawableType>>(std::forward<DrawableType>(obj)));
        }

        ObjectCollection& GetObjects() {
            return objects_;
        }

    private:
        std::vector<std::shared_ptr<IDrawable>> objects_;
        svg::Document svg_document_;
    };

    class Map {
    public:
        virtual geo::Bounds<geo::Point> GetProjectedBounds() {
            return geo::Bounds<geo::Point>{projection_.FromLatLngToMapPoint(bounds_.min), projection_.FromLatLngToMapPoint(bounds_.max)};
        }

    private:
        Size size_;
        geo::Projection projection_;
        geo::Bounds<geo::Coordinates> bounds_;
    };
}

namespace transport_catalogue::io::renderer /* IRenderer */ {

    class IRenderer {
    public:
        using Projection_ = geo::SphereProjection;
        virtual void UpdateMapProjection(Projection_&& projection) = 0;
        virtual void AddRouteToLayer(const data::BusRecord&& bus_record) = 0;
        virtual void AddStopToLayer(const data::StopRecord&& stop_record) = 0;
        virtual void SetRenderSettings(maps::RenderSettings&& settings) = 0;

        virtual maps::RenderSettings& GetRenderSettings() = 0;

        virtual svg::Document& GetMap() = 0;                   //! FOR DEBUG ONLY
        virtual svg::Document& GetRouteLayer() = 0;            //! FOR DEBUG ONLY
        virtual svg::Document& GetRouteNamesLayer() = 0;       //! FOR DEBUG ONLY
        virtual svg::Document& GetStopMarkersLayer() = 0;      //! FOR DEBUG ONLY
        virtual svg::Document& GetStopMarkerNamesLayer() = 0;  //! FOR DEBUG ONLY
        virtual ~IRenderer() = default;
    };
}

namespace transport_catalogue::maps /* MapRenderer */ {
    class MapRenderer : public io::renderer::IRenderer {
        using Projection_ = IRenderer::Projection_;

    public:
        MapRenderer()
            : color_picker_{settings_.color_palette} {}

    public:
        template <typename ObjectType>
        class IDbObject;

        class StopMarker;

        class BusRoute;

    private:
        struct LayerSet {
            MapLayer map;
            MapLayer routes;
            MapLayer route_names;
            MapLayer stop_markers;
            MapLayer stop_marker_names;
        };

        class ColorPicker {
        public:
            ColorPicker(ColorPalette color_palette)
                : route_colors_iterator_{ColorPalette(color_palette)}, busses_colors_iterator_{ColorPalette(color_palette)} {}
            Color GetRouteColor() {
                return route_colors_iterator_.GetCurrentColorOrNone();
            }
            Color GetStopColor() {
                return busses_colors_iterator_.GetCurrentColorOrNone();
            }

            void SetNextRouteColor() {
                route_colors_iterator_.NextColor();
            }
            void SetNextBusColor() {
                busses_colors_iterator_.NextColor();
            }

            void SetColorPalette(ColorPalette colors) {
                route_colors_iterator_.SetColorPalette(ColorPalette{colors});
                busses_colors_iterator_.SetColorPalette(ColorPalette{colors});
            }

        private:
            ColorPaletteСyclicIterator route_colors_iterator_;
            ColorPaletteСyclicIterator busses_colors_iterator_;
        };

    public:
        void UpdateLayers();

        void UpdateMapProjection(Projection_&& projection) override;

        void AddRouteToLayer(const data::BusRecord&& bus_record) override;

        void AddStopToLayer(const data::StopRecord&& stop_record) override;

        void SetRenderSettings(RenderSettings&& settings) override;

        RenderSettings& GetRenderSettings() override;

        svg::Document& GetMap() override;

        svg::Document& GetRouteLayer() override;

        svg::Document& GetRouteNamesLayer() override;

        svg::Document& GetStopMarkersLayer() override;

        svg::Document& GetStopMarkerNamesLayer() override;

    private:
        LayerSet layers_;
        Projection_ projection_;
        RenderSettings settings_;
        ColorPicker color_picker_;
    };
}

namespace transport_catalogue::maps /* MapRenderer::DbObject */ {
    template <typename ObjectType>
    class MapRenderer::IDbObject {
    public:
        virtual void Update() {
            if (db_record_ == data::DbNull<ObjectType>) {
                return;
            }
            throw std::runtime_error("Not implemented");
        }

        virtual void UpdateLocation() = 0;

        const std::string_view GetName() const {
            return name_;
        }

        const data::DbRecord<ObjectType>& GetDbRecord() const {
            return db_record_;
        }

    protected:
        IDbObject(data::DbRecord<ObjectType> db_record, const Projection_& projection) : db_record_{db_record}, projection_{projection} {}

    protected:
        std::string name_;
        data::DbRecord<ObjectType> db_record_;
        const Projection_& projection_;
    };
}

namespace transport_catalogue::maps /* MapRenderer::BusRoute */ {
    class MapRenderer::BusRoute : public IDbObject<data::Bus>, public IDrawable {
    public:
        class BusRouteLable;

    public:
        BusRoute(data::BusRecord bus_record, const RenderSettings& settings, const Projection_& projection)
            : IDbObject{bus_record, projection}, IDrawable{settings} {
            Build();
        }

        void Update() override {
            IDbObject::Update();
            UpdateLocation();
        }

        void UpdateLocation() override;

        void Darw(svg::ObjectContainer& layer) const override;

        const data::Route& GetRoute() const;

        void SetColor(const Color&& color);

        BusRouteLable BuildLable() const;

        virtual std::shared_ptr<IDrawable> Clone() const override final {
            return std::make_shared<MapRenderer::BusRoute>(*this);
        }

    private:
        Polyline locations_;
        Color color_ = ColorPaletteСyclicIterator::NoneColor;
        std::shared_ptr<int> ref_ = std::make_shared<int>();
        void Build() {
            assert(locations_.empty());

            const data::Route& route = db_record_->route;
            locations_.reserve(route.size());
            std::for_each(route.begin(), route.end(), [this](const data::StopRecord& stop) {
                locations_.emplace_back(projection_.FromLatLngToMapPoint(stop->coordinates), stop->coordinates);
            });
        }
    };
}

namespace transport_catalogue::maps /* MapRenderer::BusRoute::BusRouteLable */ {

    class MapRenderer::BusRoute::BusRouteLable : public IDbObject<data::Bus>, public IDrawable {
    private:
        struct NameLable {
            std::string text;
            Location location;

            NameLable(std::string text, Location location) : text(text), location(location) {}
        };

    public:
        BusRouteLable(const BusRoute& drawable_bus)
            : IDbObject{drawable_bus.db_record_, drawable_bus.projection_},
              IDrawable{drawable_bus.settings_},
              drawable_bus_{drawable_bus},
              parent_ref_handle_{drawable_bus.ref_},
              color_{drawable_bus.color_} {
            Build();
        }

        void Darw(svg::ObjectContainer& layer) const override {
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

        void Update() override {
            assert(HasValidParent());
            IDbObject::Update();
            UpdateLocation();
        }

        void UpdateLocation() override {
            Build();
        }

        bool HasValidParent() const {
            return !parent_ref_handle_.expired();
        }

        virtual std::shared_ptr<IDrawable> Clone() const override final {
            return std::make_shared<MapRenderer::BusRoute::BusRouteLable>(*this);
        }

    private:
        const BusRoute& drawable_bus_;
        std::weak_ptr<int> parent_ref_handle_;
        Color color_;
        std::vector<NameLable> name_lables_;

        void Build() {
            assert(HasValidParent());
            assert(db_record_ == drawable_bus_.db_record_);

            const auto& locations = drawable_bus_.locations_;
            assert(db_record_->route.size() == drawable_bus_.locations_.size());

            if (locations.empty()) {
                return;
            }

            const std::string& name = db_record_->name;
            name_lables_.emplace_back(name, locations.front());

            if (locations.size() > 1 && !db_record_->is_roundtrip) {
                //! assert(locations.size() % 2ul);
                auto center = static_cast<size_t>(locations.size() / 2ul);
                if (db_record_->route[center]->name != db_record_->route.front()->name) {
                    name_lables_.emplace_back(name, locations[center]);
                }
            }
        }
    };
}

namespace transport_catalogue::maps /* MapRenderer::StopMarker */ {

    class MapRenderer::StopMarker : public IDbObject<data::Stop>, public IDrawable {
    public:
        class StopMarkerLable;

    public:
        StopMarker(data::StopRecord bus_record, const RenderSettings& settings, const Projection_& projection)
            : IDbObject{bus_record, projection}, IDrawable{settings} {
            Build();
        }

        void Update() override;

        void UpdateLocation() override;

        void Darw(svg::ObjectContainer& layer) const override;

        void SetColor(const Color&& color);

        StopMarkerLable BuildLable() const;

        virtual std::shared_ptr<IDrawable> Clone() const override final;

        const Location& GetLocation() const {
            return location_;
        }

        const std::string& GetName() const {
            return db_record_->name;
        }

    private:
        Location location_;
        Color color_ = ColorPaletteСyclicIterator::NoneColor;
        std::shared_ptr<int> ref_ = std::make_shared<int>();

        void Build() {
            location_ = Location(projection_.FromLatLngToMapPoint(db_record_->coordinates), db_record_->coordinates);
        }
    };
}

namespace transport_catalogue::maps /* MapRenderer::StopMarker::StopMarkerLable */ {

    class MapRenderer::StopMarker::StopMarkerLable : public IDbObject<data::Stop>, public IDrawable {
    private:
        struct NameLable {
            std::string text;
            Location location;

            NameLable(std::string text, Location location) : text(text), location(location) {}
        };

    public:
        StopMarkerLable(const StopMarker& bus_marker)
            : IDbObject{bus_marker.db_record_, bus_marker.projection_},
              IDrawable{bus_marker.settings_},
              bus_marker_{bus_marker},
              parent_ref_handle_{bus_marker.ref_},
              color_{bus_marker.color_} {
            Build();
        }

        void Darw(svg::ObjectContainer& layer) const override {
            assert(HasValidParent());
            if (name_lables_.empty()) {
                return;
            }
            /*static const std::string font_weight("bold");
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
            });*/

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

        void Update() override {
            assert(HasValidParent());
            IDbObject::Update();
            UpdateLocation();
        }

        void UpdateLocation() override {
            Build();
        }

        bool HasValidParent() const {
            return !parent_ref_handle_.expired();
        }

        virtual std::shared_ptr<IDrawable> Clone() const override final {
            return std::make_shared<MapRenderer::StopMarker::StopMarkerLable>(*this);
        }

    private:
        const StopMarker& bus_marker_;
        std::weak_ptr<int> parent_ref_handle_;
        Color color_;
        std::vector<NameLable> name_lables_;

        void Build() {
            assert(HasValidParent());
            assert(db_record_ == bus_marker_.db_record_);

            name_lables_.emplace_back(db_record_->name, bus_marker_.location_);
        }
    };
}