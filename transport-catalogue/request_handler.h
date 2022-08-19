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
#include <array>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

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
        RawRequest(const RawRequest& other) = default;
        RawRequest& operator=(const RawRequest& other) = default;
        RawRequest(RawRequest&& other) = default;
        RawRequest& operator=(RawRequest&& other) = default;
    };

    struct Request {
    public:
        using RequestArgsMap = RawRequest;
        using Array = std::vector<RequestArrayValueType>;
        using Dict = std::unordered_map<std::string, RequestDictValueType>;

        struct TypeValues {
            inline static const std::string_view STOP = "Stop";
            inline static const std::string_view BUS = "Bus";
        };

        Request(RequestCommand type, std::string&& name, RequestArgsMap&& args)
            : command_{std::move(type)}, name_{std::move(name)}, args_{std::move(args)} {}

        Request(std::string&& type, std::string&& name, RequestArgsMap&& args);

        explicit Request(RawRequest&& raw_request);

        Request(const Request& other) = default;
        Request& operator=(const Request& other) = default;
        Request(Request&& other) = default;
        Request& operator=(Request&& other) = default;

        virtual bool IsBaseRequest() const;
        virtual bool IsStatRequest() const;

        RequestCommand& GetCommand();

        const RequestCommand& GetCommand() const;

        std::string& GetName();

        const std::string& GetName() const;

        virtual ~Request() = default;

    protected:
        RequestCommand command_ = RequestCommand::UNDEFINED;
        std::string name_;
        RequestArgsMap args_;

    protected:
        Request() = default;
        virtual void Build() {}
    };

    class BaseRequest : public Request {
        using Request::Request;

    public:
        BaseRequest(RequestCommand type, std::string&& name, RequestArgsMap&& args) : Request(std::move(type), std::move(name), std::move(args)) {
            Build();
        }
        explicit BaseRequest(RawRequest&& raw_request) : Request(std::move(raw_request)) {
            Build();
        }

        BaseRequest(const BaseRequest& other) = default;
        BaseRequest& operator=(const BaseRequest& other) = default;
        BaseRequest(BaseRequest&& other) = default;
        BaseRequest& operator=(BaseRequest&& other) = default;

        const std::vector<std::string>& GetStops() const;

        std::vector<std::string>& GetStops();

        const std::optional<bool>& IsRoundtrip() const;

        std::optional<bool>& IsRoundtrip();

        const std::optional<data::Coordinates>& GetCoordinates() const;

        std::optional<data::Coordinates>& GetCoordinates();

        const std::vector<data::MeasuredRoadDistance>& GetroadDistances() const;

        std::vector<data::MeasuredRoadDistance>& GetroadDistances();

        bool IsBaseRequest() const override;

        bool IsStatRequest() const override;

        bool IsStopCommand() const;

        bool IsBusCommand() const;

    protected:
        void Build() override;

    private:
        std::vector<std::string> stops_;
        std::optional<bool> is_roundtrip_;
        std::optional<data::Coordinates> coordinates_;
        std::vector<data::MeasuredRoadDistance> road_distances_;

    private:
        void FillBus();

        void FillStop();

        void FillStops();

        void FillRoundtrip();

        void FillCoordinates();

        void FillRoadDistances();
    };

    class StatRequest : public Request {
        using Request::Request;

    public:
        bool IsBaseRequest() const override {
            return command_ == RequestCommand::BUS;
        }

        bool IsStatRequest() const override {
            return command_ == RequestCommand::STOP;
        }

        int GetRequestId() const {
            return request_id_;
        }

    protected:
        void Build() override {}

    private:
        int request_id_;
    };
}

namespace transport_catalogue::io /* Response */ {
    using ResponseBase = RequestBase;
    class RawResponse : public ResponseBase {
        using ResponseBase::unordered_map;
    };

    class Response {
        using ResponseArgsMap = RawResponse;

    public:
        Response(std::optional<int>&& request_id, ResponseArgsMap&& args) : request_id_{std::move(request_id)}, args_{std::move(args)} {}

        std::optional<int>& GetRequestId() {
            return request_id_;
        }

        const std::optional<int>& GetRequestId() const {
            return request_id_;
        }

        ResponseArgsMap& GetArgs() {
            return args_;
        }

        const ResponseArgsMap& GetArgs() const {
            return args_;
        }

        void Print() const {}

    protected:
        std::optional<int> request_id_;
        ResponseArgsMap args_;
    };

    class StatResponse : public Response {
        using Response::Response;
    };
}

namespace transport_catalogue::io /* Interfaces */ {
    class IRequestNotifier;
    class IRequestObserver {
    public:
        virtual void OnBaseRequest(std::vector<RawRequest>&& requests) = 0;
        virtual void OnBaseRequest(const std::vector<RawRequest>& requests) = 0;
        virtual void OnStatRequest(std::vector<RawRequest>&& requests) = 0;
        virtual void OnStatRequest(const std::vector<RawRequest>& requests) = 0;

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
        virtual void NotifyBaseRequest(std::vector<RawRequest>&& requests) = 0;
        virtual void NotifyStatRequest(std::vector<RawRequest>&& requests) = 0;
    };

    class IStatResponseSender {
    public:
        virtual bool Send(StatResponse&& response) const = 0;
        virtual ~IStatResponseSender() = default;
    };
}

namespace transport_catalogue::io {

    class RequestHandler : public IRequestObserver {
    public:
        // MapRenderer понадобится в следующей части итогового проекта
        RequestHandler(
            const data::ITransportStatDataReader& reader, const data::ITransportDataWriter& writer, const IStatResponseSender& response_sender,
            const io::renderer::MapRenderer& renderer)
            : db_reader_{reader}, db_writer_{writer}, response_sender_{response_sender}, renderer_{renderer} {}

        ~RequestHandler() {
#if (TRACE && REQUEST_TRACE && TRACE_CTR)
            std::cerr << "Stop request handler" << std::endl;
#endif
        };

        // Возвращает информацию о маршруте (запрос Bus)
        std::optional<data::BusStat> GetBusStat(const std::string_view bus_name) const;

        // Возвращает маршруты, проходящие через
        const data::BusRecordSet& GetBusesByStop(const std::string_view& stop_name) const;

        // Этот метод будет нужен в следующей части итогового проекта
        svg::Document RenderMap() const;

        void OnBaseRequest(std::vector<RawRequest>&& requests) override;

        void OnBaseRequest(const std::vector<RawRequest>& requests) override;

        void OnStatRequest([[maybe_unused]] std::vector<RawRequest>&& requests) override;

        void OnStatRequest([[maybe_unused]] const std::vector<RawRequest>& requests) override;

        /// Execute Basic (Insert) request
        void ExecuteRequest(BaseRequest&& raw_req, std::vector<data::MeasuredRoadDistance>& out_distances) const;

        /// Execute Basic (Insert) requests
        void ExecuteRequest(std::vector<BaseRequest>&& base_req) const;

        void ExecuteRequest(StatRequest&& stat_req) const;

        void SendStatResponse(StatResponse&& response) const;

    private:
        // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
        const data::ITransportStatDataReader& db_reader_;
        const data::ITransportDataWriter& db_writer_;
        const IStatResponseSender& response_sender_;
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