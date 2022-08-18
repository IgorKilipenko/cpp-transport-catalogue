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

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <variant>

#include "domain.h"
#include "map_renderer.h"
#include "svg.h"
#include "transport_catalogue.h"

namespace transport_catalogue::exceptions {
    class RequestParsingException : public std::logic_error {
    public:
        template <typename String = std::string>
        RequestParsingException(String&& message) : std::runtime_error(std::forward<String>(message)) {}
    };
}

namespace transport_catalogue::io /* Requests */ {
    using RequestArrayValueType = std::variant<std::monostate, std::string, int, double, bool>;
    using RequestDictValueType = std::variant<std::monostate, std::string, int, double, bool>;
    using RequestValueType = std::variant<
        std::monostate, std::string, int, double, bool, std::vector<RequestArrayValueType>, std::unordered_map<std::string, RequestDictValueType>>;
    using RequestBase = std::unordered_map<std::string, RequestValueType>;

    enum class RequestType : u_int8_t { BASE, STAT };
    enum class RequestCommand : uint8_t { STOP, BUS, UNDEFINED };

    class RawRequest : public RequestBase {
        using RequestBase::unordered_map;

    public:
        RawRequest(const RawRequest& other) = delete;
        RawRequest& operator=(const RawRequest& other) = delete;
        RawRequest(RawRequest&& other) = default;
        RawRequest& operator=(RawRequest&& other) = default;
    };

    struct Request {
    public:
        using RequestArgsMap = RawRequest;

        struct TypeValues {
            inline static const std::string_view STOP = "Stop";
            inline static const std::string_view BUS = "Bus";
        };

        RequestCommand type = RequestCommand::UNDEFINED;
        std::string name;

        Request(RequestCommand type, std::string&& name, RequestArgsMap&& args) : type{std::move(type)}, name{std::move(name)}, args_{std::move(args)} {}
        Request(std::string&& type, std::string&& name, RequestArgsMap&& args)
            : Request(
                  (assert(type == TypeValues::BUS || type == TypeValues::STOP),
                   TypeValues::BUS == std::move(type) ? RequestCommand::BUS : RequestCommand::STOP),
                  std::move(name), std::move(args)) {}

        explicit Request(RawRequest&& raw_request)
            : Request(
                  (assert(raw_request.count("type") && std::holds_alternative<std::string>(raw_request.at("type"))),
                   std::get<std::string>(std::move(raw_request.extract("type").mapped()))),
                  (assert(raw_request.count("name")), std::get<std::string>(std::move(raw_request.extract("name").mapped()))),
                  ((assert(raw_request.size() > 0), std::move(raw_request)))) {}

        /*Request(Request&& other) : Request(std::move(other.type), std::move(other.name), std::move(other.args_)) {
            std::cerr << "Request move constructor" << std::endl;
        }
        Request& operator=(Request&& other) {
            if (this == &other) {
                return *this;
            }
            RequestArgsMap args_tmp;
            args_tmp.swap(other.args_);

            type = other.type;
            name = std::move(other.name);

            return *this;
        }*/

        Request(const Request& other) = delete;
        Request& operator=(const Request& other) = delete;
        Request(Request&& other) = default;
        Request& operator=(Request&& other) = default;

        virtual ~Request() = default;

        virtual void Build() {}

    protected:
        Request() = default;
        RequestArgsMap args_;
    };

    class BaseRequest : public Request {
        using Request::Request;

    public:
        // BaseRequest() = default;
        // BaseRequest(Type type, std::string&& name, RequestArgsMap&& args) : Request(type, std::move(name), std::move(args)) {}
        // BaseRequest(RawRequest&& raw_request)
        //    : Request(
        //          (assert(raw_request.count("type") && std::holds_alternative<std::string>(raw_request.at("type"))),
        //           std::get<std::string>(std::move(raw_request.extract("type").mapped()))),
        //          (assert(raw_request.count("name")), std::get<std::string>(std::move(raw_request.extract("name").mapped())))),
        //      args_((assert(raw_request.size() > 0), std::move(raw_request))) {}

        std::vector<std::string> stops;
        std::optional<bool> is_roundtrip;
        std::optional<data::Coordinates> coordinates;
        std::optional<data::MeasuredRoadDistance> road_distances;

        BaseRequest(const BaseRequest& other) = delete;
        BaseRequest& operator=(const BaseRequest& other) = delete;
        BaseRequest(BaseRequest&& other) = default;
        BaseRequest& operator=(BaseRequest&& other) = default;

        void Build() override {
            assert(args_.size());
            assert(type == RequestCommand::BUS || type == RequestCommand::STOP);

            if (type == RequestCommand::BUS) {
                auto stops_ptr = args_.find("stops");
                std::vector<RequestArrayValueType> stops_tmp =
                    stops_ptr == args_.end() ? std::vector<RequestArrayValueType>{}
                                             : std::get<std::vector<RequestArrayValueType>>(
                                                   (assert(std::holds_alternative<std::vector<RequestArrayValueType>>(stops_ptr->second)),
                                                    std::move(args_.extract(stops_ptr).mapped())));
                stops.resize(stops_tmp.size());
                std::transform(stops_tmp.begin(), stops_tmp.end(), stops.begin(), [&](auto&& stop_name) {
                    assert(std::holds_alternative<std::string>(stop_name));
                    return std::get<std::string>(stop_name);
                });

                auto is_roundtrip_ptr = args_.find("is_roundtrip");
                is_roundtrip = is_roundtrip_ptr == args_.end() ? std::optional<bool>(
                                                                     (assert(std::holds_alternative<bool>(is_roundtrip_ptr->second)),
                                                                      std::get<bool>(std::move(args_.extract(is_roundtrip_ptr).mapped()))))
                                                               : std::nullopt;
            } else {
                auto latitude_ptr = args_.find("latitude");
                std::optional<double> latitude = latitude_ptr == args_.end() ? std::optional<double>((
                                                                                   assert(std::holds_alternative<double>(latitude_ptr->second)),
                                                                                   std::get<double>(std::move(args_.extract(latitude_ptr).mapped()))))
                                                                             : std::nullopt;
                auto longitude_ptr = args_.find("longitude");
                std::optional<double> longitude = longitude_ptr == args_.end()
                                                      ? std::optional<double>(
                                                            (assert(std::holds_alternative<double>(longitude_ptr->second)),
                                                             std::get<double>(std::move(args_.extract(longitude_ptr).mapped()))))
                                                      : std::nullopt;
                assert((latitude.has_value() && longitude.has_value()) || !(latitude.has_value() && longitude.has_value()));
                coordinates = latitude.has_value() ? std::nullopt : std::optional<Coordinates>({longitude.value(), latitude.value()});
            }
        }
    };
}

namespace transport_catalogue::io /* Interfaces */ {
    class IRequestNotifier;
    class IRequestObserver {
    public:
        virtual void OnBaseRequest(std::vector<Request>&& requests) = 0;
        virtual void OnStatRequest(std::vector<Request>&& requests) = 0;

    protected:
        virtual ~IRequestObserver() = default;
    };

    class IRequestNotifier {
    public:
        virtual void AddObserver(std::shared_ptr<IRequestObserver> observer) = 0;
        virtual void RemoveObserver(std::shared_ptr<IRequestObserver> observer) = 0;
        virtual ~IRequestNotifier() = default;

    protected:
        virtual bool HasObserver() const = 0;
        virtual void NotifyBaseRequest(std::vector<Request>&& requests) = 0;
        virtual void NotifyStatRequest(std::vector<Request>&& requests) = 0;
    };
}

namespace transport_catalogue::io {

    class RequestHandler : public IRequestObserver {
    public:
        // MapRenderer понадобится в следующей части итогового проекта
        RequestHandler(const data::ITransportStatDataReader& reader, const io::renderer::MapRenderer& renderer)
            : db_reader_{reader}, renderer_{renderer} {}

        ~RequestHandler() {
            std::cerr << "Stop request handler" << std::endl;  //!!! FOR DEBUG
        };

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

        void OnBaseRequest(std::vector<Request>&& requests) override {
            std::cerr << "onBaseRequest" << std::endl;
            std::sort(requests.begin(), requests.end(), [](const Request& lhs, const Request& rhs) {
                assert(lhs.type != RequestCommand::UNDEFINED && rhs.type != RequestCommand::UNDEFINED);
                return (lhs.type) < (rhs.type);
            });
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

namespace transport_catalogue::io {
    class RequestParser {
    public:
        inline static const std::string TYPE_FIELD = "type";
        inline static const std::string NAME_FIELD = "name";

        void ParseRawRequest(std::vector<RawRequest>&& requests) const {
            assert(!requests.empty());
        }
        void ParseBaseRequest(RawRequest&& request) const {
            assert(!request.empty());
            assert(request.count(TYPE_FIELD));
            assert(request.count(NAME_FIELD));
        }
        bool CheckScheme(const RawRequest& request) const {
            return request.count(TYPE_FIELD) && request.count(NAME_FIELD);
        }
    };
}