#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
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

#include "detail/type_traits.h"
#include "domain.h"
#include "geo.h"
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

namespace transport_catalogue::io /* Requests aliases */ {
    using RequestArrayValueType = std::variant<std::monostate, std::string, int, double, bool>;
    using RequestDictValueType = std::variant<std::monostate, std::string, int, double, bool>;
    using RequestValueType = std::variant<
        std::monostate, std::string, int, double, bool, std::vector<RequestArrayValueType>, std::unordered_map<std::string, RequestDictValueType>>;
    using RequestBase = std::unordered_map<std::string, RequestValueType>;

    enum class RequestType : int8_t { BASE, STAT, RENDER_SETTINGS, UNKNOWN };
    enum class RequestCommand : uint8_t { STOP, BUS, MAP, UNKNOWN };
    struct RequestFields {
        inline static const std::string BASE_REQUESTS{"base_requests"};
        inline static const std::string STAT_REQUESTS{"stat_requests"};
        inline static const std::string RENDER_SETTINGS{"render_settings"};
        inline static const std::string TYPE{"type"};
        inline static const std::string NAME{"name"};
        inline static const std::string STOPS{"stops"};
        inline static const std::string IS_ROUNDTRIP{"is_roundtrip"};
        inline static const std::string LATITUDE{"latitude"};
        inline static const std::string LONGITUDE{"longitude"};
        inline static const std::string ROAD_DISTANCES{"road_distances"};
        inline static const std::string ID{"id"};
    };
}

namespace transport_catalogue::io /* RequestEnumConverter */ {
    struct RequestEnumConverter {
        inline static std::string InvalidValue{"Invalid enum value"};

        template <typename EnumType>
        std::string_view operator()(EnumType enum_value) const {
            throw std::invalid_argument(InvalidValue);
        }

        template <typename EnumType>
        EnumType operator()(std::string_view enum_name) const {
            throw std::invalid_argument(InvalidValue);
        }

        template <>
        std::string_view operator()(RequestCommand enum_value) const;

        template <>
        RequestCommand operator()(std::string_view enum_name) const;

        template <>
        std::string_view operator()(RequestType enum_value) const;

        template <>
        RequestType operator()(std::string_view enum_name) const;
    };
}

namespace transport_catalogue::io /* RawRequest */ {

    /// Выполняет роль адаптера транспорта запросов : IRequestNotifier --> RequestHandler
    class RawRequest : public RequestBase {
        using RequestBase::unordered_map;

    public:
        RawRequest(const RawRequest& other) = default;
        RawRequest& operator=(const RawRequest& other) = default;
        RawRequest(RawRequest&& other) = default;
        RawRequest& operator=(RawRequest&& other) = default;
    };
}

namespace transport_catalogue::io /* Requests */ {

    struct Request {
    public: /* Aliases */
        using RequestArgsMap = RawRequest;
        using Array = std::vector<RequestArrayValueType>;
        using Dict = std::unordered_map<std::string, RequestDictValueType>;

    public:
        Request(const Request& other) = default;
        Request& operator=(const Request& other) = default;
        Request(Request&& other) = default;
        Request& operator=(Request&& other) = default;

        virtual bool IsBaseRequest() const;
        virtual bool IsStatRequest() const;
        virtual bool IsRenderSettingsRequest() const;
        virtual bool IsValidRequest() const;

        virtual bool IsStopCommand() const {
            return command_ == RequestCommand::STOP;
        }

        virtual bool IsBusCommand() const {
            return command_ == RequestCommand::BUS;
        }

        virtual bool IsMapCommand() const {
            return command_ == RequestCommand::MAP;
        }

        RequestCommand& GetCommand();

        const RequestCommand& GetCommand() const;

        std::string& GetName();

        const std::string& GetName() const;

        virtual ~Request() = default;

    protected:
        RequestCommand command_ = RequestCommand::UNKNOWN;
        std::string name_;
        RequestArgsMap args_;

    protected:
        inline static const RequestEnumConverter converter{};
        Request() = default;
        Request(RequestCommand type, std::string&& name, RequestArgsMap&& args)
            : command_{std::move(type)}, name_{std::move(name)}, args_{std::move(args)} {}

        Request(std::string&& type, std::string&& name, RequestArgsMap&& args);

        explicit Request(RawRequest&& raw_request);
        virtual void Build() {
            assert(command_ != RequestCommand::MAP || !name_.empty());
        }
    };

    class BaseRequest : public Request {
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

        bool IsRoundtrip() const;

        const std::optional<data::Coordinates>& GetCoordinates() const;

        std::optional<data::Coordinates>& GetCoordinates();

        const std::vector<data::MeasuredRoadDistance>& GetroadDistances() const;

        std::vector<data::MeasuredRoadDistance>& GetroadDistances();

        bool IsBaseRequest() const override;

        bool IsValidRequest() const override;

        void ConvertToRoundtrip();

        static void ConvertToRoundtrip(std::vector<std::string>& stops);

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
    public:
        StatRequest(RequestCommand type, std::string&& name, RequestArgsMap&& args) : Request(std::move(type), std::move(name), std::move(args)) {
            Build();
        }
        explicit StatRequest(RawRequest&& raw_request) : Request(std::move(raw_request)) {
            Build();
        }

        bool IsBaseRequest() const override;

        bool IsStatRequest() const override;

        bool IsValidRequest() const override;

        const std::optional<int>& GetRequestId() const;

        std::optional<int>& GetRequestId();

    protected:
        void Build() override;

    private:
        std::optional<int> request_id_;
    };

    class RenderSettingsRequest : public Request {
    public:
        RenderSettingsRequest(RequestCommand type, std::string&& name, RequestArgsMap&& args)
            : Request(std::move(type), std::move(name), std::move(args)) {
            Build();
        }
        explicit RenderSettingsRequest(RawRequest&& raw_request) : Request(std::move(raw_request)) {
            Build();
        }

        bool IsBaseRequest() const override;

        bool IsStatRequest() const override;

        bool IsValidRequest() const override;

        const std::optional<int>& GetRequestId() const;

        std::optional<int>& GetRequestId();

    protected:
        void Build() override;

    private:
        std::optional<int> request_id_;
    };
}

namespace transport_catalogue::io /* Response */ {
    using ResponseBase = RequestBase;
    class RawResponse : public ResponseBase {
        using ResponseBase::unordered_map;
    };

    class Response {
    public:
        using ResponseArgsMap = RawResponse;
        Response(int&& request_id, RequestCommand&& command, std::string&& name)
            : request_id_{std::move(request_id)}, command_{std::move(command)}, name_{std::move(name)} {}

        int& GetRequestId();

        int GetRequestId() const;

        virtual bool IsBusResponse() const;

        virtual bool IsStopResponse() const;

        virtual bool IsMapResponse() const;

        virtual bool IsStatResponse() const;

        virtual bool IsBaseResponse() const;

    protected:
        int request_id_;
        RequestCommand command_;
        std::string name_;
    };

    class StatResponse final : public Response {
    public:
        StatResponse(
            int&& request_id, RequestCommand&& command, std::string&& name, std::optional<data::BusStat>&& bus_stat = std::nullopt,
            std::optional<data::StopStat>&& stop_stat = std::nullopt);

        StatResponse(
            StatRequest&& request, std::optional<data::BusStat>&& bus_stat = std::nullopt, std::optional<data::StopStat>&& stop_stat = std::nullopt);

        std::optional<data::BusStat>& GetBusInfo();

        std::optional<data::StopStat>& GetStopInfo();

        bool IsStatResponse() const override;

    private:
        std::optional<data::BusStat> bus_stat_;
        std::optional<data::StopStat> stop_stat_;
    };
}

namespace transport_catalogue::io /* Interfaces */ {
    class IRequestObserver {
    public:
        virtual void OnBaseRequest(std::vector<RawRequest>&& requests) const = 0;
        virtual void OnStatRequest(std::vector<RawRequest>&& requests) const = 0;
        virtual void OnRenderSettingsRequest(RawRequest&& requests) const = 0;

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
        virtual void NotifyRenderSettingsRequest(RawRequest&& requests) = 0;
    };

    class IStatResponseSender {
    public:
        virtual bool Send(StatResponse&& response) const = 0;
        virtual size_t Send(std::vector<StatResponse>&& responses) const = 0;
        virtual ~IStatResponseSender() = default;
    };
}

namespace transport_catalogue::io /* RequestHandler */ {

    class RequestHandler : public IRequestObserver {
    public:
        // MapRenderer понадобится в следующей части итогового проекта
        RequestHandler(
            const data::ITransportStatDataReader& reader, const data::ITransportDataWriter& writer, const IStatResponseSender& response_sender,
            io::renderer::IRenderer& renderer)
            : db_reader_{reader}, db_writer_{writer}, response_sender_{response_sender}, renderer_{renderer} {}

        ~RequestHandler() {
#if (TRACE && REQUEST_TRACE && TRACE_CTR)
            std::cerr << "Stop request handler" << std::endl;
#endif
        };

        void RenderMap(maps::RenderSettings settings) const {
            const auto& all_stops = db_reader_.GetDataReader().GetStopsTable();
            std::vector<geo::Coordinates> points{all_stops.size()};
            std::transform(all_stops.begin(), all_stops.end(), points.begin(), [](const auto& stop) {
                return stop.coordinates;
            });
            geo::MockProjection projection = geo::MockProjection::CalculateFromParams(std::move(points), {settings.map_size}, settings.padding);

            renderer_.UpdateMapProjection(projection);
            const data::BusRecord bus = db_reader_.GetDataReader().GetBus(all_stops.front().name);
            renderer_.DrawTransportTracksLayer(bus);
        }

        void OnBaseRequest(std::vector<RawRequest>&& requests) const override;

        void OnStatRequest(std::vector<RawRequest>&& requests) const override;

        void OnRenderSettingsRequest(RawRequest&& requests) const override;

        /// Execute Basic (Insert) request
        void ExecuteRequest(BaseRequest&& raw_req, std::vector<data::MeasuredRoadDistance>& out_distances) const;

        /// Execute Basic (Insert) requests
        void ExecuteRequest(std::vector<BaseRequest>&& base_req) const;

        /// Execute Stat (Get) request
        void ExecuteRequest(StatRequest&& stat_req) const;

        /// Execute Stat (Get) requests
        void ExecuteRequest(std::vector<StatRequest>&& stat_req) const;

        /// Send Stat Response
        void SendStatResponse(StatResponse&& response) const;

        /// Send Stat Response
        void SendStatResponse(std::vector<StatResponse>&& responses) const;

    private:
        // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
        const data::ITransportStatDataReader& db_reader_;
        const data::ITransportDataWriter& db_writer_;
        const IStatResponseSender& response_sender_;
        [[maybe_unused]] io::renderer::IRenderer& renderer_;
    };
}