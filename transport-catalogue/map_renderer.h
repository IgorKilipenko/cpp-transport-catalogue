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

        void Transform(const geo::Projection& projection);

        const MapPoint& GetMapPoint() const ;

        MapPoint& GetMapPoint() ;

        const GlobalCoordinates& GetGlobalCoordinates() const;

        GlobalCoordinates& GetGlobalCoordinates() ;

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

namespace transport_catalogue::maps /* ColorPaletteСyclicIterator */ {

    class ColorPaletteСyclicIterator {
    public:
        inline static const ColorPalette::value_type NoneColor = svg::Colors::NoneColor;

        ColorPaletteСyclicIterator() : ColorPaletteСyclicIterator(ColorPalette()) {}
        ColorPaletteСyclicIterator(ColorPalette&& palette) : palette_(std::move(palette)), curr_it_{palette_.begin()} {}

        ColorPalette::const_iterator NextColor();

        const ColorPalette& GetColorPalette() const;

        const ColorPalette::value_type& GetCurrentColor() const;

        const ColorPalette::value_type& GetPrevColor() const;

        const ColorPalette::value_type& GetCurrentColorOrNone() const noexcept;

        const ColorPalette::value_type& GetPrevColorOrNone() const noexcept;

        bool IsEmpty() const;

        void SetColorPalette(ColorPalette&& new_palette);

    private:
        ColorPalette palette_;
        ColorPalette::iterator curr_it_;
    };
}

namespace transport_catalogue::maps /* RenderSettings */ {

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

        svg::Document& GetSvgDocument();

        svg::Document&& ExtractSvgDocument();

        void Draw();

        template <typename DrawableType>
        void Add(DrawableType&& obj) {
            objects_.emplace_back(std::make_shared<std::decay_t<DrawableType>>(std::forward<DrawableType>(obj)));
        }

        ObjectCollection& GetObjects();

    private:
        std::vector<std::shared_ptr<IDrawable>> objects_;
        svg::Document svg_document_;
    };
}

namespace transport_catalogue::io::renderer /* IRenderer */ {

    class IRenderer {
    public:
        using Projection_ = geo::SphereProjection;
        using RawMapData = std::string;

        virtual void UpdateMapProjection(Projection_&& projection) = 0;
        virtual void AddRouteToLayer(const data::BusRecord&& bus_record) = 0;
        virtual void AddStopToLayer(const data::StopRecord&& stop_record) = 0;
        virtual void SetRenderSettings(maps::RenderSettings&& settings) = 0;

        virtual maps::RenderSettings& GetRenderSettings() = 0;
        virtual RawMapData GetRawMap() = 0;
        virtual svg::Document& GetRouteLayer() = 0;
        virtual svg::Document& GetRouteNamesLayer() = 0;
        virtual svg::Document& GetStopMarkersLayer() = 0;
        virtual svg::Document& GetStopMarkerNamesLayer() = 0;

        virtual ~IRenderer() = default;
    };
}

namespace transport_catalogue::maps /* MapRenderer */ {
    class MapRenderer : public io::renderer::IRenderer {
        using Projection_ = IRenderer::Projection_;

    public:
        using RawMapData = IRenderer::RawMapData;

    public:
        MapRenderer() : color_picker_{settings_.color_palette} {}

    public:
        template <typename ObjectType>
        class IDbObject;

        class StopMarker;

        class BusRoute;

    private:
        struct LayerSet {
            MapLayer routes;
            MapLayer route_names;
            MapLayer stop_markers;
            MapLayer stop_marker_names;

            void DrawMap();

            RawMapData ExtractRawMapData();
        };

        class ColorPicker {
        public:
            ColorPicker(ColorPalette color_palette)
                : route_colors_iterator_{ColorPalette(color_palette)}, busses_colors_iterator_{ColorPalette(color_palette)} {}

            Color GetRouteColor();

            Color GetStopColor();

            void SetNextRouteColor();

            void SetNextBusColor();

            void SetColorPalette(ColorPalette colors);

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

        RawMapData GetRawMap() override;

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

        void Update() override;

        void UpdateLocation() override;

        void Darw(svg::ObjectContainer& layer) const override;

        const data::Route& GetRoute() const;

        void SetColor(const Color&& color);

        BusRouteLable BuildLable() const;

        virtual std::shared_ptr<IDrawable> Clone() const override final;

    private:
        Polyline locations_;
        Color color_ = ColorPaletteСyclicIterator::NoneColor;
        std::shared_ptr<int> ref_ = std::make_shared<int>();

        void Build();
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

        void Darw(svg::ObjectContainer& layer) const override;

        void Update() override;

        void UpdateLocation() override;

        bool HasValidParent() const;

        virtual std::shared_ptr<IDrawable> Clone() const override final;

    private:
        const BusRoute& drawable_bus_;
        std::weak_ptr<int> parent_ref_handle_;
        Color color_;
        std::vector<NameLable> name_lables_;

        void Build();
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

        const Location& GetLocation() const;

        const std::string& GetName() const;

    private:
        Location location_;
        Color color_ = ColorPaletteСyclicIterator::NoneColor;
        std::shared_ptr<int> ref_ = std::make_shared<int>();

        void Build();
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

        void Darw(svg::ObjectContainer& layer) const override;

        void Update() override;

        void UpdateLocation() override;

        bool HasValidParent() const;

        virtual std::shared_ptr<IDrawable> Clone() const override final;

    private:
        const StopMarker& bus_marker_;
        std::weak_ptr<int> parent_ref_handle_;
        Color color_;
        std::vector<NameLable> name_lables_;

        void Build();
    };
}