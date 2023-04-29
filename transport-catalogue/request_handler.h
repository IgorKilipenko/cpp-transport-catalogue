#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "detail/type_traits.h"
#include "domain.h"
#include "geo.h"
#include "map_renderer.h"
#include "serialization.h"
#include "svg.h"
#include "transport_catalogue.h"
#include "transport_router.h"

namespace transport_catalogue::exceptions {
    class RequestParsingException : public std::logic_error {
    public:
        template <typename String = std::string>
        RequestParsingException(String&& message) : std::runtime_error(std::forward<String>(message)) {}
    };
}

namespace transport_catalogue::io /* Requests aliases */ {
    using RawMapData = maps::MapRenderer::RawMapData;
    using RouteInfo = router::RouteInfo;
    using RequestInnerArrayValueType = std::variant<std::monostate, std::string, int, double, bool>;
    using RequestArrayValueType = std::variant<std::monostate, std::string, int, double, bool, std::vector<RequestInnerArrayValueType>>;
    using RequestDictValueType = std::variant<std::monostate, std::string, int, double, bool, std::vector<RequestInnerArrayValueType>>;

    using RequestValueTypeBase = std::variant<
        std::monostate, std::string, int, double, bool, std::vector<RequestArrayValueType>, std::unordered_map<std::string, RequestDictValueType>>;

    class RequestValueType : public RequestValueTypeBase {
    public:
        using RequestValueTypeBase::variant;
        using ValueType = RequestValueTypeBase;
        using AtomicValueType = std::variant<std::monostate, std::string, int, double, bool>;
        using Color = std::variant<std::string, std::vector<std::variant<uint8_t, double>>>;
        using Array = std::vector<RequestArrayValueType>;
        using Dict = std::vector<RequestDictValueType>;
        using InnerArray = std::vector<RequestInnerArrayValueType>;
        using Offset = std::array<double, 2>;

        bool IsArray() const;
        bool IsDictionary() const;
        std::optional<double> ExtractNumericIf();
        std::optional<Color> ExtractColorIf();

        template <typename... Args>
        static std::optional<std::variant<uint8_t, double>> ExtractRgbaColorItemIf(std::variant<Args...>&& value);

        // clang-format off
        template <typename ValType, detail::EnableIf<!std::is_lvalue_reference_v<ValType> 
            && (detail::IsConvertibleV<ValType, Array::value_type> ||detail::IsConvertibleV<ValType, RequestValueType::ValueType>)> = true>
        static std::optional<Color> ExtractColorIf(ValType&& value);

        template <typename ValType, detail::EnableIf<!std::is_lvalue_reference_v<ValType> 
            && (detail::IsConvertibleV<ValType, Array::value_type> || detail::IsConvertibleV<ValType, RequestValueType::ValueType>)> = true>
        static std::optional<double> ExtractNumericIf(ValType&& value);
        // clang-format on
    };

    using RequestBase = std::unordered_map<std::string, RequestValueType>;
}

namespace transport_catalogue::io /* Request fields enums */ {
    enum class RequestType : int8_t { BASE, STAT, RENDER_SETTINGS, ROUTING_SETTINGS, SERIALIZATION_SETTINGS, UNKNOWN };

    /// Request GET commands (for build responses)
    enum class RequestCommand : uint8_t { STOP, BUS, MAP, ROUTE, UNKNOWN };

    struct RequestFields {
        inline static const std::string BASE_REQUESTS{"base_requests"};
        inline static const std::string STAT_REQUESTS{"stat_requests"};
        inline static const std::string RENDER_SETTINGS{"render_settings"};
        inline static const std::string ROUTING_SETTINGS{"routing_settings"};
        inline static const std::string SERIALIZATION_SETTINGS{"serialization_settings"};
        inline static const std::string TYPE{"type"};
        inline static const std::string NAME{"name"};
    };

    struct BaseRequestFields {
        inline static const std::string TYPE = RequestFields::TYPE;
        inline static const std::string NAME = RequestFields::NAME;
        inline static const std::string STOPS{"stops"};
        inline static const std::string IS_ROUNDTRIP{"is_roundtrip"};
        inline static const std::string LATITUDE{"latitude"};
        inline static const std::string LONGITUDE{"longitude"};
        inline static const std::string ROAD_DISTANCES{"road_distances"};
    };

    struct StatRequestFields {
        inline static const std::string TYPE = RequestFields::TYPE;
        inline static const std::string NAME = RequestFields::NAME;
        inline static const std::string ID{"id"};
    };

    struct RenderSettingsRequestFields {
        inline static const std::string WIDTH{"width"};
        inline static const std::string HEIGHT{"height"};
        inline static const std::string PADDING{"padding"};
        inline static const std::string STOP_RADIUS{"stop_radius"};
        inline static const std::string LINE_WIDTH{"line_width"};
        inline static const std::string BUS_LABEL_FONT_SIZE{"bus_label_font_size"};
        inline static const std::string BUS_LABEL_OFFSET{"bus_label_offset"};
        inline static const std::string STOP_LABEL_FONT_SIZE{"stop_label_font_size"};
        inline static const std::string STOP_LABEL_OFFSET{"stop_label_offset"};
        inline static const std::string UNDERLAYER_WIDTH{"underlayer_width"};
        inline static const std::string UNDERLAYER_COLOR{"underlayer_color"};
        inline static const std::string COLOR_PALETTE{"color_palette"};
    };

    struct RoutingSettingsRequestFields {
        inline static const std::string BUS_WAIT_TIME{"bus_wait_time"};
        inline static const std::string BUS_VELOCITY{"bus_velocity"};
    };

    struct SerializationSettingsFields {
        inline static const std::string FILE{"file"};
    };
}

namespace transport_catalogue::io /* RequestEnumConverter */ {
    struct RequestEnumConverter {
        inline static std::string InvalidValue{"Invalid enum value"};

        std::string_view operator()(RequestCommand enum_value) const;
        std::string_view operator()(RequestType enum_value) const;

        RequestType ToRequestType(std::string_view enum_name) const;
        RequestCommand ToRequestCommand(std::string_view enum_name) const;
    };
}

namespace transport_catalogue::io /* RawRequest */ {

    /// Выполняет роль адаптера транспорта запросов : IRequestNotifier --> RequestHandler
    class RawRequest : public RequestBase {
    public:
        using RequestBase::unordered_map;
        using ValueType = RequestBase::mapped_type;
        using ItemType = RequestBase::value_type;
        using KeyType = RequestBase::key_type;
        using Array = std::vector<RequestArrayValueType>;
        using InnerArray = std::vector<RequestInnerArrayValueType>;
        using Dict = std::unordered_map<std::string, RequestDictValueType>;
        using NullValue = std::monostate;
        using Color = ValueType::Color;
        using Offset = ValueType::Offset;

    public:
        RawRequest(const RawRequest& other) = default;
        RawRequest& operator=(const RawRequest& other) = default;
        RawRequest(RawRequest&& other) = default;
        RawRequest& operator=(RawRequest&& other) = default;

        template <
            typename ReturnType, typename KeyType_,
            detail::EnableIf<detail::IsConvertibleV<ReturnType, ValueType> && !detail::IsSameV<ReturnType, ValueType>> = true>
        ReturnType Extract(KeyType_&& key) noexcept;

        template <
            typename ReturnType, typename KeyType_,
            detail::EnableIf<detail::IsConvertibleV<ReturnType, ValueType> && !detail::IsSameV<ReturnType, ValueType>> = true>
        std::optional<ReturnType> ExtractIf(KeyType_&& key) noexcept;

        template <
            typename ReturnType, typename KeyType_,
            detail::EnableIf<detail::IsConvertibleV<ReturnType, ValueType> && !detail::IsSameV<ReturnType, ValueType>> = true>
        ReturnType ExtractWithThrow(KeyType_&& key);

        template <typename KeyType_>
        std::optional<double> ExtractNumberValueIf(KeyType_&& key);

        template <typename KeyType_>
        std::optional<Offset> ExtractOffsetValueIf(KeyType_&& key);

        template <typename KeyType_>
        std::optional<Color> ExtractColorValueIf(KeyType_&& key);

        template <typename KeyType_>
        std::optional<std::vector<Color>> ExtractColorPaletteIf(KeyType_&& key);

        template <typename Filter_ = ValueType, typename... Args>
        static ValueType VariantCast(std::variant<Args...>&& value);

        template <typename Filter_ = Array, typename... Args>
        static std::optional<Array> CastToArray(std::variant<Args...>&& value);
    };
}

namespace transport_catalogue::io /* Request */ {

    class Request {
    public: /* Aliases */
        using RequestArgsMap = RawRequest;
        using InnerArray = std::vector<RequestInnerArrayValueType>;
        using Array = std::vector<RequestArrayValueType>;
        using Dict = std::unordered_map<std::string, RequestDictValueType>;

    public:
        Request(const Request& other) = default;
        Request& operator=(const Request& other) = default;
        Request(Request&& other) = default;
        Request& operator=(Request&& other) = default;

    public: /* Request type check */
        virtual bool IsBaseRequest() const;
        virtual bool IsStatRequest() const;
        virtual bool IsRenderSettingsRequest() const;
        virtual bool IsRoutingSettingsRequest() const;
        virtual bool IsSerializationSettingsRequest() const;
        virtual bool IsValidRequest() const;

    public: /* Commands for build response */
        virtual bool IsGetStopCommand() const;
        virtual bool IsGetBusCommand() const;
        virtual bool IsGetMapCommand() const;
        virtual bool IsGetRouteCommand() const;

        RequestCommand& GetCommand();
        const RequestCommand& GetCommand() const;

        virtual ~Request() = default;

    protected:
        RequestCommand command_ = RequestCommand::UNKNOWN;
        RequestArgsMap args_;

    protected:
        inline static const RequestEnumConverter converter{};
        Request() = default;
        Request(RequestCommand type, RequestArgsMap&& args) : command_{std::move(type)}, args_{std::move(args)} {}
        Request(std::string&& type, RequestArgsMap&& args);

        explicit Request(RawRequest&& raw_request);
        virtual void Build() {
            assert((command_ != RequestCommand::MAP && command_ != RequestCommand::ROUTE));
        }
    };
}

namespace transport_catalogue::io /* BaseRequest */ {
    class BaseRequest : public Request {
    public:
        BaseRequest(RequestCommand type, std::string&& name, RequestArgsMap&& args)
            : Request(std::move(type), std::move(args)), name_(std::move(name)) {
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
        bool IsConvertedToRoundtrip() const;
        const std::optional<data::Coordinates>& GetCoordinates() const;
        std::optional<data::Coordinates>& GetCoordinates();
        const std::vector<data::MeasuredRoadDistance>& GetRoadDistances() const;
        std::vector<data::MeasuredRoadDistance>& GetRoadDistances();
        bool IsBaseRequest() const override;
        bool IsValidRequest() const override;
        void ConvertToRoundtrip();
        static void ConvertToRoundtrip(std::vector<std::string>& stops);
        std::string& GetName();
        const std::string& GetName() const;

    protected:
        void Build() override;

    private:
        std::string name_;
        std::vector<std::string> stops_;
        std::optional<bool> is_roundtrip_;
        std::optional<data::Coordinates> coordinates_;
        std::vector<data::MeasuredRoadDistance> road_distances_;
        bool is_converted_to_roundtrip_ = false;

    private:
        void FillBus_();
        void FillStop_();
        void FillStops_();
        void FillRoundtrip_();
        void FillCoordinates_();
        void FillRoadDistances_();
    };
}

namespace transport_catalogue::io /* StatRequest */ {
    class IStatRequest {
    public:
        virtual const std::optional<int>& GetRequestId() const = 0;
        virtual std::optional<int>& GetRequestId() = 0;
        virtual ~IStatRequest() {}
    };

    class StatRequest : public Request, public IStatRequest {
    public:
        StatRequest(RequestCommand type, std::string&& name, RequestArgsMap&& args)
            : Request(std::move(type), std::move(args)), name_{std::move(name)} {
            Build();
        }
        explicit StatRequest(RawRequest&& raw_request) : Request(std::move(raw_request)) {
            Build();
        }

        virtual bool IsBaseRequest() const override;
        virtual bool IsStatRequest() const override;
        virtual bool IsValidRequest() const override;
        virtual const std::optional<int>& GetRequestId() const override;
        virtual std::optional<int>& GetRequestId() override;
        virtual std::optional<std::string>& GetName();
        virtual const std::optional<std::string>& GetName() const;

    protected:
        virtual void Build() override;

    private:
        std::optional<int> request_id_;
        std::optional<std::string> name_;
    };
}

namespace transport_catalogue::io /* RouteStatRequest */ {

    class RouteStatRequest final : public StatRequest {
        using StatRequest::StatRequest;

    public:
        RouteStatRequest(StatRequest&& request);

        bool IsValidRequest() const override;
        const std::optional<std::string>& GetFromStop() const;
        std::optional<std::string>& GetFromStop();
        const std::optional<std::string>& GetToStop() const;
        std::optional<std::string>& GetToStop();

    private:
        std::optional<std::string> from_;
        std::optional<std::string> to_;

    private:
        void Build() override;
    };
}

namespace transport_catalogue::io /* RenderSettingsRequest */ {

    class RenderSettingsRequest : public Request {
    public:
        using Color = RawRequest::Color;
        using Offset = RawRequest::Offset;

    public:
        RenderSettingsRequest(RequestCommand type, RequestArgsMap&& args);
        explicit RenderSettingsRequest(RawRequest&& raw_request);

        bool IsValidRequest() const override;
        bool IsRenderSettingsRequest() const override;

        std::optional<double>& GetWidth();
        std::optional<double>& GetHeight();
        std::optional<double>& GetPadding();
        std::optional<double>& GetStopRadius();
        std::optional<double>& GetLineWidth();
        std::optional<int>& GetBusLabelFontSize();
        std::optional<Offset>& GetBusLabelOffset();
        std::optional<int>& GetStopLabelFontSize();
        std::optional<Offset>& GetStopLabelOffset();
        std::optional<double>& GetUnderlayerWidth();
        std::optional<Color>& GetUnderlayerColor();
        std::optional<std::vector<Color>>& GetColorPalette();

    protected:
        void Build() override;

    private:
        std::optional<double> width_;
        std::optional<double> height_;
        std::optional<double> padding_;
        std::optional<double> stop_radius_;
        std::optional<double> line_width_;

        std::optional<int> bus_label_font_size_;
        std::optional<Offset> bus_label_offset_;

        std::optional<int> stop_label_font_size_;
        std::optional<Offset> stop_label_offset_;

        std::optional<double> underlayer_width_;
        std::optional<Color> underlayer_color_;

        std::optional<std::vector<Color>> color_palette_;
    };
}

namespace transport_catalogue::io /* RoutingSettingsRequest */ {

    class RoutingSettingsRequest : public Request {
    public:
        RoutingSettingsRequest(RequestCommand type, RequestArgsMap&& args);
        explicit RoutingSettingsRequest(RawRequest&& raw_request);

        bool IsValidRequest() const override;

        const std::optional<uint16_t>& GetBusWaitTimeMin() const;
        const std::optional<uint16_t>& GetBusVelocityKmh() const;
        bool IsRoutingSettingsRequest() const override;

    protected:
        void Build() override;

    private:
        std::optional<uint16_t> bus_wait_time_min_;
        std::optional<uint16_t> bus_velocity_kmh_;
    };
}

namespace transport_catalogue::io /* SerializationSettingsRequest */ {

    class SerializationSettingsRequest : public Request {
    public:
        explicit SerializationSettingsRequest(RawRequest&& raw_request);

        const std::optional<std::string>& GetFile() const;
        bool IsSerializationSettingsRequest() const override;
        bool IsValidRequest() const override;

    public:
        using Fields = SerializationSettingsFields;

    protected:
        void Build() override;

    private:
        std::optional<std::string> file_;
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
        virtual bool IsRouteResponse() const;
        virtual bool IsStatResponse() const;
        virtual bool IsBaseResponse() const;

    protected:
        int request_id_;
        RequestCommand command_;
        std::string name_;
    };

    class StatResponse final : public Response {
    public:
        StatResponse(StatRequest&& request) : StatResponse(std::move(request), std::nullopt, std::nullopt, std::nullopt) {}

        StatResponse(
            int&& request_id, RequestCommand&& command, std::string&& name, std::optional<data::BusStat>&& bus_stat = std::nullopt,
            std::optional<data::StopStat>&& stop_stat = std::nullopt, std::optional<RawMapData>&& map_data = std::nullopt,
            std::optional<RouteInfo>&& route_info = std::nullopt);

        StatResponse(
            StatRequest&& request, std::optional<data::BusStat>&& bus_stat = std::nullopt, std::optional<data::StopStat>&& stop_stat = std::nullopt,
            std::optional<RawMapData>&& map_data = std::nullopt, std::optional<RouteInfo>&& route_info = std::nullopt);

        std::optional<data::BusStat>& GetBusInfo();
        std::optional<data::StopStat>& GetStopInfo();
        std::optional<RawMapData>& GetMapData();
        std::optional<RouteInfo>& GetRouteInfo();

        bool IsStatResponse() const override;

    private:
        std::optional<data::BusStat> bus_stat_;
        std::optional<data::StopStat> stop_stat_;
        std::optional<RawMapData> map_data_;
        std::optional<RouteInfo> route_info_;
    };
}

namespace transport_catalogue::io /* Interfaces */ {
    class IRequestObserver {
    public:
        virtual void OnReadingComplete(RawRequest&& request) = 0;
        virtual void OnBaseRequest(std::vector<RawRequest>&& requests) = 0;
        virtual void OnStatRequest(std::vector<RawRequest>&& requests) = 0;
        virtual void OnRenderSettingsRequest(RawRequest&& requests) = 0;
        virtual void OnRoutingSettingsRequest(RawRequest&& requests) = 0;
        virtual void OnSerializationSettingsRequest(RawRequest&& request) = 0;

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
        virtual void NotifyReadingComplete(bool complete) = 0;
        virtual void NotifyBaseRequest(std::vector<RawRequest>&& requests) = 0;
        virtual void NotifyStatRequest(std::vector<RawRequest>&& requests) = 0;
        virtual void NotifyRenderSettingsRequest(RawRequest&& requests) = 0;
        virtual void NotifyRoutingSettingsRequest(RawRequest&& requests) = 0;
        virtual void NotifySerializationSettingsRequest(RawRequest&& requests) = 0;
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
    public: /* Constants */
        enum class Mode { MAKE_BASE, PROCESS_REQUESTS };

    public:
        RequestHandler(
            const data::ITransportStatDataReader& reader, const data::ITransportDataWriter& writer, const IStatResponseSender& response_sender,
            io::renderer::IRenderer& renderer, Mode mode = Mode::PROCESS_REQUESTS)
            : db_reader_{reader},
              db_writer_{writer},
              response_sender_{response_sender},
              renderer_{renderer},
              router_({}, db_reader_.GetDataReader()),
              storage_(db_reader_, db_writer_, renderer_),
              mode_{mode} {}

        ~RequestHandler() {}

        class RenderSettingsBuilder;

        //* Is non-const for use cache in next versions
        std::string RenderMap();

        /// Build (or if force_prepare_data = false get pre-builded) map and return non-rendered layers (as svg documents).
        ///! Used for testing only
        std::vector<svg::Document*> RenderMapByLayers(bool force_prepare_data = false);

        void OnReadingComplete(RawRequest&& request) override;
        void OnBaseRequest(std::vector<RawRequest>&& requests) override;
        void OnStatRequest(std::vector<RawRequest>&& requests) override;
        void OnRenderSettingsRequest(RawRequest&& requests) override;
        void OnRoutingSettingsRequest(RawRequest&& requests) override;
        void OnSerializationSettingsRequest(RawRequest&& request) override;

        /// Execute Basic (Insert) request
        void ExecuteRequest(BaseRequest&& raw_req, std::vector<data::MeasuredRoadDistance>& out_distances) const;

        /// Execute Basic (Insert) requests
        void ExecuteRequest(std::vector<BaseRequest>&& base_req);

        /// Execute Stat (Get) request
        void ExecuteRequest(StatRequest&& stat_req);

        /// Execute Stat (Get) requests
        void ExecuteRequest(std::vector<StatRequest>&& stat_req);

        /// Execute RenderSettings (Set) request
        void ExecuteRequest(RenderSettingsRequest&& request);

        void ExecuteRequest(RoutingSettingsRequest&& request);

        /// Execute SerializationSettings (Set) request
        void ExecuteRequest(SerializationSettingsRequest&& request);

        /// Send Stat Response
        void SendStatResponse(StatResponse&& response) const;

        /// Send Stat Response
        void SendStatResponse(std::vector<StatResponse>&& responses) const;

    private:
        const data::ITransportStatDataReader& db_reader_;
        const data::ITransportDataWriter& db_writer_;
        const IStatResponseSender& response_sender_;
        io::renderer::IRenderer& renderer_;
        router::TransportRouter router_;
        serialization::Store storage_;
        Mode mode_;

        bool PrepareMapRendererData();
    };
}

namespace transport_catalogue::io /* RequestHandler::SettingsBuilder */ {

    class RequestHandler::RenderSettingsBuilder {
    public:
        static maps::RenderSettings BuildMapRenderSettings(RenderSettingsRequest&& request);

        template <typename RawColor_, detail::EnableIf<!std::is_lvalue_reference_v<RawColor_>> = true>
        static std::optional<maps::Color> ConvertColor(RawColor_&& raw_color);
    };
}

namespace transport_catalogue::io /* RequestValueType template implementation */ {

    template <typename... Args>
    std::optional<std::variant<uint8_t, double>> RequestValueType::ExtractRgbaColorItemIf(std::variant<Args...>&& value) {
        return std::visit(
            [](auto&& arg) -> std::optional<std::variant<uint8_t, double>> {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (detail::IsConvertibleV<T, std::variant<int, double>>) {
                    if constexpr (detail::IsSameV<T, int>) {
                        assert(arg >= 0 && arg <= std::numeric_limits<uint8_t>::max());
                        return static_cast<uint8_t>(std::move(arg));
                    } else {
                        assert(arg >= 0. && arg <= 1.);
                        return std::move(arg);
                    }
                } else {
                    return std::nullopt;
                }
            },
            value);
    }

    template <
        typename ValType, detail::EnableIf<
                              !std::is_lvalue_reference_v<ValType> && (detail::IsConvertibleV<ValType, RequestValueType::Array::value_type> ||
                                                                       detail::IsConvertibleV<ValType, RequestValueType::ValueType>)>>
    std::optional<RequestValueType::Color> RequestValueType::ExtractColorIf(ValType&& value) {
        std::string* string_ptr = std::get_if<std::string>(&value);
        if (string_ptr != nullptr) {
            assert(!string_ptr->empty());

            return std::move(*string_ptr);
        }

        const auto convert = [](auto&& rgb) -> std::vector<std::variant<uint8_t, double>> {
            assert(rgb.size() >= 3 && rgb.size() <= 4);
            std::vector<std::variant<uint8_t, double>> rgb_result(rgb.size());
            std::transform(std::make_move_iterator(rgb.begin()), std::make_move_iterator(rgb.end()), rgb_result.begin(), [](auto&& arg) {
                assert(std::holds_alternative<int>(arg) || std::holds_alternative<double>(arg));
                auto rgb_item = ExtractRgbaColorItemIf(std::move(arg));
                if (!rgb_item.has_value()) {
                    throw std::invalid_argument("Invalid RGB(a) color item");
                }
                return rgb_item.value();
            });
            return rgb_result;
        };

        if constexpr (detail::IsConvertibleV<ValType, Array::value_type>) {
            InnerArray* rgb_ptr = std::get_if<InnerArray>(std::move(&value));
            if (rgb_ptr == nullptr) {
                return std::nullopt;
            }
            return convert(std::move(*rgb_ptr));

        } else if constexpr (detail::IsConvertibleV<ValType, RequestValueType::ValueType>) {
            Array* rgb_ptr = std::get_if<Array>(std::move(&value));
            if (rgb_ptr == nullptr) {
                return std::nullopt;
            }
            return convert(std::move(*rgb_ptr));
        }

        return std::nullopt;
    }

    template <
        typename ValType, detail::EnableIf<
                              !std::is_lvalue_reference_v<ValType> && (detail::IsConvertibleV<ValType, RequestValueType::Array::value_type> ||
                                                                       detail::IsConvertibleV<ValType, RequestValueType::ValueType>)>>
    std::optional<double> RequestValueType::ExtractNumericIf(ValType&& value) {
        const double* double_ptr = std::get_if<double>(&value);
        if (double_ptr != nullptr) {
            return std::move(*double_ptr);
        }

        const int* int_ptr = std::get_if<int>(&value);
        if (int_ptr != nullptr) {
            return std::move(*int_ptr);
        }
        return std::nullopt;
    }

}

namespace transport_catalogue::io /* RawRequest template implementation */ {

    template <
        typename ReturnType, typename KeyType_,
        detail::EnableIf<detail::IsConvertibleV<ReturnType, RawRequest::ValueType> && !detail::IsSameV<ReturnType, RawRequest::ValueType>>>
    ReturnType RawRequest::Extract(KeyType_&& key) noexcept {
        auto it = find(std::forward<KeyType_>(key));
        if (it == end()) {
            return {};
        }
        assert(std::holds_alternative<ReturnType>(it->second));

        return std::get<ReturnType>(std::move(extract(it).mapped()));
    }

    template <
        typename ReturnType, typename KeyType_,
        detail::EnableIf<detail::IsConvertibleV<ReturnType, RawRequest::ValueType> && !detail::IsSameV<ReturnType, RawRequest::ValueType>>>
    std::optional<ReturnType> RawRequest::ExtractIf(KeyType_&& key) noexcept {
        auto ptr = find(std::forward<KeyType_>(key));

        if (ptr == end()) {
            return std::nullopt;
        }

        assert(std::holds_alternative<ReturnType>(ptr->second));

        auto mapped = std::move(extract(ptr).mapped());
        auto* result_ptr = std::get_if<ReturnType>(std::move(&mapped));
        return result_ptr ? std::optional<ReturnType>(std::move(*result_ptr)) : std::nullopt;
    }

    template <
        typename ReturnType, typename KeyType_,
        detail::EnableIf<detail::IsConvertibleV<ReturnType, RawRequest::ValueType> && !detail::IsSameV<ReturnType, RawRequest::ValueType>>>
    ReturnType RawRequest::ExtractWithThrow(KeyType_&& key) {
        assert(std::holds_alternative<ReturnType>(at(key)));
        return std::get<ReturnType>(std::move(extract(key).mapped()));
    }

    template <typename KeyType_>
    std::optional<double> RawRequest::ExtractNumberValueIf(KeyType_&& key) {
        auto it = find(std::forward<KeyType_>(key));
        if (it == end()) {
            return std::nullopt;
        }
        ValueType value = std::move(extract(it).mapped());
        return value.ExtractNumericIf();
    }

    template <typename KeyType_>
    std::optional<RawRequest::Color> RawRequest::ExtractColorValueIf(KeyType_&& key) {
        auto it = find(std::forward<KeyType_>(key));
        if (it == end()) {
            return std::nullopt;
        }
        ValueType value = std::move(extract(it).mapped());
        auto color = value.ExtractColorIf();
        return color;
    }

    template <typename KeyType_>
    std::optional<std::vector<RawRequest::Color>> RawRequest::ExtractColorPaletteIf(KeyType_&& key) {
        auto array = ExtractIf<Array>(std::forward<KeyType_>(key));
        if (!array.has_value()) {
            return std::nullopt;
        }

        std::vector<Color> color_palette;
        color_palette.reserve(array.value().size());
        std::for_each(
            std::make_move_iterator(array.value().begin()), std::make_move_iterator(array.value().end()), [&color_palette](auto&& raw_color) {
                std::optional<Color> color = RawRequest::ValueType::ExtractColorIf(std::move(raw_color));
                if ((assert(color.has_value()), color.has_value())) {
                    color_palette.emplace_back(std::move(color.value()));
                }
            });

        return color_palette;
    }

    template <typename Filter_, typename... Args>
    RawRequest::ValueType RawRequest::VariantCast(std::variant<Args...>&& value) {
        return std::visit(
            [](auto&& arg) -> ValueType {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (detail::IsConvertibleV<T, Filter_>) {
                    return std::move(arg);
                } else {
                    return NullValue();
                }
            },
            value);
    }

    template <typename Filter_, typename... Args>
    std::optional<RawRequest::Array> RawRequest::CastToArray(std::variant<Args...>&& value) {
        return std::visit(
            [](auto&& arg) -> std::optional<Array> {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (detail::IsConvertibleV<T, Filter_>) {
                    return std::move(arg);
                } else {
                    return std::nullopt;
                }
            },
            value);
    }

    template <typename KeyType_>
    std::optional<RawRequest::Offset> RawRequest::ExtractOffsetValueIf(KeyType_&& key) {
        std::optional<Array> array = ExtractIf<Array>(std::forward<KeyType_>(key));
        if (!array.has_value() || (assert(array->size() == 2), array->size() != 2)) {
            return std::nullopt;
        }

        auto first_val = ValueType::ExtractNumericIf(std::move(array->front()));
        auto second_val = ValueType::ExtractNumericIf(std::move(array->back()));

        return Offset{first_val.value(), second_val.value()};
    }
}

namespace transport_catalogue::io /* RequestHandler::RenderSettingsBuilder template implementation */ {

    template <typename RawColor_, detail::EnableIf<!std::is_lvalue_reference_v<RawColor_>>>
    std::optional<maps::Color> RequestHandler::RenderSettingsBuilder::ConvertColor(RawColor_&& raw_color) {
        return svg::Colors::ConvertColor(std::move(raw_color));
    }
}
