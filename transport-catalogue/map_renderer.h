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
#include <iterator>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "detail/type_traits.h"
#include "domain.h"
#include "geo.h"
#include "svg.h"
#include "transport_catalogue.h"

namespace transport_catalogue::maps /* Aliases */ {
    using Color = svg::Color;
    using Offset = geo::Offset;
    using Size = geo::Size;
    using GlobalCoordinates = geo::Coordinates;

    class Style {
    public:
        Color color;
        double stroke_width = 0.;
    };

    class TextStyle : public Style {
    public:
        Offset offset;
        uint32_t font_size;
        std::optional<Color> underlayer_color;
    };
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

    struct RenderSettings {
        /*
        "width": 9289.364936976961,
        "height": 9228.70241453055,
        "padding": 512.5688364782626,
        "stop_radius": 24676.062211914188,
        "line_width": 79550.25988723598,
        "stop_label_font_size": 79393,
        "stop_label_offset": [-2837.3688837120862, 55117.27307444796],
        "underlayer_color": [82, 175, 153, 0.11467454568298674],
        "underlayer_width": 98705.69087384189,
        */
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

        std::vector<Color> color_palette;
    };

    class Drawable {
    public:
        virtual void Darw(svg::ObjectContainer& layer) const = 0;
        virtual ~Drawable() = default;

    protected:
        Drawable(const RenderSettings& settings) : settings_(settings) {}
        /*Drawable(Style style) : style_(style) {}
        Drawable(TextStyle style) : text_style_(style) {}
        Drawable(Style style, TextStyle text_style) : style_(style), text_style_(text_style) {}*/

    protected:
        // std::optional<Style> style_;
        // std::optional<TextStyle> text_style_;
        const RenderSettings& settings_;
    };
}

namespace transport_catalogue::maps {
    class MapLayer {
    public:
        svg::Document& GetSvgDocument() {
            return svg_document_;
        }

        void Draw() {
            std::for_each(objects_.begin(), objects_.end(), [this](Drawable& obj) {
                obj.Darw(svg_document_);
            });
        }

    private:
        std::vector<Drawable> objects_;
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
        using Projection_ = geo::MockProjection;
        virtual void UpdateMapProjection(Projection_&& projection) = 0;
        // virtual void DrawTransportTracksLayer(std::vector<data::BusRecord>&& records) = 0;
        virtual void DrawTransportTracksLayer(data::BusRecord bus) = 0;
        virtual void DrawTransportTracksLayer(std::vector<data::BusRecord>&& records) = 0;
        virtual void DrawTransportStopsLayer(std::vector<data::StopRecord>&& records) = 0;
        virtual void SetRenderSettings(maps::RenderSettings&& settings) = 0;
        virtual maps::RenderSettings& GetRenderSettings() = 0;
        virtual svg::Document& GetMap() = 0;  //! FOR DEBUG ONLY
        virtual ~IRenderer() = default;
    };
}

namespace transport_catalogue::maps /* MapRenderer */ {
    class MapRenderer : public io::renderer::IRenderer {
        using Projection_ = IRenderer::Projection_;

    public:
        template <typename ObjectType>
        class DbObject {
        public:
            virtual void Update() {
                if (db_id_ == data::DbNull<ObjectType>) {
                    return;
                }
                throw std::runtime_error("Not implemented");
            }

            virtual void UpdateLocation() = 0;

            const std::string_view GetName() const {
                return name_;
            }

            const data::DbRecord<ObjectType>& GetDbRecord() const {
                return db_id_;
            }

        protected:
            DbObject(data::DbRecord<ObjectType> object_id, const Projection_& projection) : db_id_{object_id}, projection_{projection} {}

        protected:
            std::string name_;
            data::DbRecord<ObjectType> db_id_;
            const Projection_& projection_;
        };

        class BusStop : public DbObject<data::Stop>, public Drawable {
        public:
            BusStop(const RenderSettings& settings, const Projection_& projection)
                : DbObject{data::DbNull<data::Stop>, projection}, Drawable{settings} {}
            BusStop(data::StopRecord id, const RenderSettings& settings, const Projection_& projection)
                : DbObject{id, projection}, Drawable{settings} {}

            void Update() override {
                DbObject::Update();
            }

            void UpdateLocation() override {
                std::cerr << "BusRoute -> Update Location" << std::endl;  //! FOR DEBUG ONLY
                location_.GetMapPoint() = {projection_.FromLatLngToMapPoint(location_.GetGlobalCoordinates())};
            }

            void Darw(svg::ObjectContainer& /*layer*/) const override {
                // layer.Add(Polyline{})
            }

        private:
            Location location_;
        };

        class BusRoute : public DbObject<data::Bus>, public Drawable {
        public:
            BusRoute(const RenderSettings& settings, const Projection_& projection)
                : DbObject{data::DbNull<data::Bus>, projection}, Drawable{settings} {
                Build();
            }
            BusRoute(data::BusRecord id, const RenderSettings& settings, const Projection_& projection)
                : DbObject{id, projection}, Drawable{settings} {
                Build();
            }

            void Update() override {
                DbObject::Update();
            }

            void UpdateLocation() override {
                // std::cerr << "BusRoute -> Update Location" << std::endl;  //! FOR DEBUG ONLY
                std::for_each(locations_.begin(), locations_.end(), [this](Location& location) {
                    location.GetMapPoint() = {projection_.FromLatLngToMapPoint(location.GetGlobalCoordinates())};
                });
            }

            void Darw(svg::ObjectContainer& layer) const override {
                // layer.Add(svg::Polyline(static_cast<std::vector<svg::Point>>(locations_)));
                layer.Add(svg::Polyline(locations_)
                              .SetFillColor(svg::NoneColor)
                              .SetStrokeWidth(settings_.line_width)
                              .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                              .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND));
            }
            void Darw(svg::ObjectContainer& layer, size_t color_index) const {
                // layer.Add(svg::Polyline(static_cast<std::vector<svg::Point>>(locations_)));
                layer.Add(svg::Polyline(locations_)
                              .SetFillColor(svg::NoneColor)
                              .SetStrokeWidth(settings_.line_width)
                              .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                              .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
                              .SetStrokeColor(settings_.color_palette[color_index]));
            }

            const data::Route& GetRoute() const {
                return db_id_->route;
            }

        private:
            Polyline locations_;
            void Build() {
                assert(locations_.empty());

                const data::Route& route = db_id_->route;
                locations_.reserve(route.size());
                std::for_each(route.begin(), route.end(), [this](const data::StopRecord& stop) {
                    locations_.emplace_back(projection_.FromLatLngToMapPoint(stop->coordinates), stop->coordinates);
                });
            }
        };

        void UpdateLayers() {
            transport_layer_.Draw();
        }

        void UpdateMapProjection(Projection_&& projection) override {
            if (projection == projection_) {
                return;
            }
            projection_ = std::move(projection);
            UpdateLayers();
        }

        void DrawTransportTracksLayer(data::BusRecord bus) override {
            assert(!settings_.color_palette.empty());
            BusRoute drawable_bus{bus, settings_, projection_};
            drawable_bus.Darw(transport_layer_.GetSvgDocument(), color_index_);
            color_index_ = color_index_ < settings_.color_palette.size() - 1ul ? color_index_ + 1ul : 0ul;
        }

        void DrawTransportTracksLayer(std::vector<data::BusRecord>&& /*records*/) override {}

        void DrawTransportStopsLayer(std::vector<data::StopRecord>&& /*records*/) override {}

        void SetRenderSettings(RenderSettings&& settings) override {
            settings_ = std::move(settings);
            //! UpdateLayers();
        }

        RenderSettings& GetRenderSettings() override {
            return settings_;
        }

        svg::Document& GetMap() override {
            UpdateLayers();
            return transport_layer_.GetSvgDocument();
        }

    private:
        MapLayer transport_layer_;
        Projection_ projection_;
        RenderSettings settings_;
        size_t color_index_ = 0;
    };
}
