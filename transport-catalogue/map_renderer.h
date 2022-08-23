/*
 * В этом файле вы можете разместить код, отвечающий за визуализацию карты маршрутов в формате SVG.
 * Визуализация маршрутов вам понадобится во второй части итогового проекта.
 * Пока можете оставить файл пустым.
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <iterator>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "domain.h"
#include "geo.h"
#include "svg.h"
#include "transport_catalogue.h"

namespace transport_catalogue::io::renderer /* MapRenderer */ {
    class IRenderer {
    public:
        // virtual void DrawTransportTracksLayer(std::vector<data::BusRecord>&& records) = 0;
        virtual void DrawTransportTracksLayer(data::BusRecord bus) = 0;
        virtual void DrawTransportStopsLayer(std::vector<data::BusRecord>&& records) = 0;
        virtual ~IRenderer() = default;
    };
}

namespace transport_catalogue::maps /* Aliases */ {
    using Color = svg::Color;
    using Offset = geo::Offset;
    using Size = geo::Size;
    using GlobalCoordinates = geo::Coordinates;
}

namespace transport_catalogue::maps /* Locations */ {

    struct MapPoint : public geo::Point {
        using geo::Point::Point;
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

        std::vector<MapPoint> ExtractPoints() {
            std::vector<MapPoint> result(size());
            std::transform(std::make_move_iterator(begin()), std::make_move_iterator(end()), result.begin(), [](Location&& location) {
                return static_cast<MapPoint>(std::move(location.GetMapPoint()));
            });
            return result;
        }
    };
}

namespace transport_catalogue::maps {
    class MapLayer {
    public:
        svg::Document& GetSvgDocument() {
            return svg_document_;
        }

    private:
        svg::Document svg_document_;
    };

    class Map {
    public:
        virtual geo::Bounds<geo::Point> GetProjectedBounds() {
            [[maybe_unused]] geo::Point min = projection_.FromLatLngToMapPoint(bounds_.min);
            [[maybe_unused]] geo::Point max = projection_.FromLatLngToMapPoint(bounds_.max);
            return geo::Bounds<geo::Point>{min, max};
        }

    private:
        Size size_;
        geo::Projection projection_;
        geo::Bounds<geo::Coordinates> bounds_;
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
        Size size;

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
}

namespace transport_catalogue::maps /* MapRenderer */ {
    class MapRenderer : public io::renderer::IRenderer {
    public:
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

        class Drawable {
        protected:
            Drawable(Style style) : style_(style) {}
            Drawable(TextStyle style) : text_style_(style) {}
            Drawable(Style style, TextStyle text_style) : style_(style), text_style_(text_style) {}
            virtual ~Drawable() = default;

            virtual void Darw(MapLayer& layer, bool move = false) const = 0;

        protected:
            std::optional<Style> style_;
            std::optional<TextStyle> text_style_;
        };

        template <typename ObjectType>
        class DbObject {
        public:
            virtual void Update() {
                if (db_id_ == data::DbNull<ObjectType>) {
                    return;
                }
                throw std::runtime_error("Not implemented");
            }

        protected:
            DbObject(data::DbRecord<ObjectType> object_id) : db_id_{object_id} {}

        protected:
            std::string name_;
            data::DbRecord<ObjectType> db_id_;
        };

        class BusStop : public DbObject<data::Stop>, public Drawable {
        public:
            BusStop(Style style) : DbObject{data::DbNull<data::Stop>}, Drawable{style} {}
            BusStop(data::StopRecord id, Style style) : DbObject{id}, Drawable{style} {}

            void Update() override {
                DbObject::Update();
            }

            void Darw(MapLayer& layer, bool move) const override {
                // layer.Add(Polyline{})
            }

        private:
            Location location_;
        };

        class BusRoute : public DbObject<data::Bus>, public Drawable {
        public:
            BusRoute(Style style) : DbObject{data::DbNull<data::Bus>}, Drawable{style} {}
            BusRoute(data::BusRecord id, Style style) : DbObject{id}, Drawable{style} {}

            void Update() override {
                DbObject::Update();
            }

            void Darw(MapLayer& layer, bool move) const override {
                // layer.Add(svg::Polyline(static_cast<std::vector<svg::Point>>(locations_)));
                svg::Document& doc = layer.GetSvgDocument();
                doc.Add(svg::Polyline(move ? std::move(locations_) : locations_));
            }

        private:
            Polyline locations_;
        };

        void DrawTransportTracksLayer(data::BusRecord bus) override {
            /*BusRoute drawable {
                bus, {}
            }*/
        }

        void DrawTransportStopsLayer(std::vector<data::BusRecord>&& records) override {}

    private:
    };
}
