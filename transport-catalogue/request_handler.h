#pragma once

/*
 * Здесь можно было бы разместить код обработчика запросов к базе, содержащего логику, которую не
 * хотелось бы помещать ни в transport_catalogue, ни в json reader.
 *
 * В качестве источника для идей предлагаем взглянуть на нашу версию обработчика запросов.
 * Вы можете реализовать обработку запросов способом, который удобнее вам.
 *
 * Если вы затрудняетесь выбрать, что можно было бы поместить в этот файл,
 * можете оставить его пустым.
 */

// Класс RequestHandler играет роль Фасада, упрощающего взаимодействие JSON reader-а
// с другими подсистемами приложения.
// См. паттерн проектирования Фасад: https://ru.wikipedia.org/wiki/Фасад_(шаблон_проектирования)

#include <optional>

#include "domain.h"
#include "map_renderer.h"
#include "svg.h"
#include "transport_catalogue.h"

namespace transport_catalogue::io {
    class RequestHandler {
    public:
        using BusRouteView = std::unordered_set<data::ConstBusPtr>;

        // MapRenderer понадобится в следующей части итогового проекта
        RequestHandler(const data::ITransportDataReader& reader, const io::renderer::MapRenderer& renderer) : reader_{reader}, renderer_{renderer} {}

        // Возвращает информацию о маршруте (запрос Bus)
        std::optional<data::BusStat> GetBusStat(const std::string_view bus_name) const {
            data::BusStat info;
            double route_length = 0;
            double pseudo_length = 0;
            const data::Bus* bus = reader_.GetBus(bus_name);
            const data::Route& route = bus->route;

            for (auto i = 0; i < route.size() - 1; ++i) {
                const data::Stop* from_stop = reader_.GetStop(route[i]->name);
                const data::Stop* to_stop = reader_.GetStop(route[i + 1]->name);
                assert(from_stop && from_stop);

                const auto& [measured_dist, dist] = reader_.GetDistanceBetweenStops(from_stop, to_stop);
                route_length += measured_dist;
                pseudo_length += dist;
            }

            info.total_stops = route.size();
            info.unique_stops = std::unordered_set<const data::Stop*>(route.begin(), route.end()).size();
            info.route_length = route_length;
            info.route_curvature = route_length / std::max(pseudo_length, 1.);

            return info;
        }

        // Возвращает маршруты, проходящие через
        const BusRouteView* GetBusesByStop(const std::string_view& stop_name) const;

        // Этот метод будет нужен в следующей части итогового проекта
        svg::Document RenderMap() const;

    private:
        // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
        const data::ITransportDataReader& reader_;
        const renderer::MapRenderer& renderer_;
    };
}