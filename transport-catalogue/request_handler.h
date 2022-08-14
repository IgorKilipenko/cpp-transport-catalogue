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

#include <functional>
#include <optional>
#include <unordered_map>

#include "domain.h"
#include "map_renderer.h"
#include "svg.h"
#include "transport_catalogue.h"

namespace transport_catalogue::io {
    using RequestValueType = std::variant<std::monostate, std::string, int, double, bool, std::vector<std::string>>;
    using RequestBase = std::unordered_map<std::string, RequestValueType>;
    class Request : public RequestBase {
        using RequestBase::unordered_map;
    };

    class IRequestObserver {
    public:
        virtual void OnBaseRequest(std::vector<Request>&& requests) = 0;
        virtual void OnStatRequest(std::vector<Request>&& requests) = 0;
        virtual ~IRequestObserver() = default;
    //protected:
    //    virtual ~IRequestObserver() = default;
    };

    class IRequestNotifier {
    public:
        virtual void SetObserver(IRequestObserver*) = 0;
        virtual bool HasObserver() const = 0;
        virtual void NotifyBaseRequest(std::vector<Request>&& requests) const = 0;
        virtual void NotifyStatRequest(std::vector<Request>&& requests) const = 0;
        virtual ~IRequestNotifier() = default;
    };
    class RequestHandler : public IRequestObserver {
    public:
        // MapRenderer понадобится в следующей части итогового проекта
        RequestHandler(const data::ITransportStatDataReader& reader, const io::renderer::MapRenderer& renderer)
            : db_reader_{reader}, renderer_{renderer} {}

        // Возвращает информацию о маршруте (запрос Bus)
        std::optional<data::BusStat> GetBusStat(const std::string_view bus_name) const {
            return db_reader_.GetBusInfo(bus_name);
        }

        // Возвращает маршруты, проходящие через
        const data::BusRecordSet& GetBusesByStop(const std::string_view& stop_name) const {
            return db_reader_.GetDataReader().GetBuses(stop_name);
        }

        // Этот метод будет нужен в следующей части итогового проекта
        svg::Document RenderMap() const;

        void OnBaseRequest([[maybe_unused]] std::vector<Request>&& requests) override {
            std::cerr << "onBaseRequest" << std::endl;
        }

        void OnStatRequest([[maybe_unused]] std::vector<Request>&& requests) override {
            std::cerr << "onStatRequest" << std::endl;
        }

    private:
        // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
        const data::ITransportStatDataReader& db_reader_;
        const renderer::MapRenderer& renderer_;
    };
}