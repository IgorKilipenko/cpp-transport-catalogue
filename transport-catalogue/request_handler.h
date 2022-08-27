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
        using InnerArray = std::vector<RequestInnerArrayValueType>;

        bool IsArray() const {
            return std::holds_alternative<Array>(*this);
        }

        bool IsDictionary() const {
            return std::holds_alternative<std::unordered_map<std::string, RequestDictValueType>>(*this);
        }

        std::optional<Color> ExtractColorIf() {
            std::string* string_ptr = std::get_if<std::string>(this);
            if (string_ptr != nullptr) {
                assert(!string_ptr->empty());

                return std::move(*string_ptr);
            }

            Array* rgb_ptr = std::get_if<Array>(std::move(this));
            if (rgb_ptr != nullptr) {
                assert(rgb_ptr->size() >= 3 && rgb_ptr->size() <= 4);
                std::vector<std::variant<uint8_t, double>> rgb_result(rgb_ptr->size());
                std::transform(
                    std::make_move_iterator(rgb_ptr->begin()), std::make_move_iterator(rgb_ptr->end()), rgb_result.begin(), [](auto&& arg) {
                        assert(std::holds_alternative<int>(arg) || std::holds_alternative<double>(arg));
                        auto rgb_item = std::visit(
                            [](auto&& arg) -> std::variant<uint8_t, double> {
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
                                    throw std::invalid_argument("Invalid Rgb[a] item");
                                }
                            },
                            arg);
                        return rgb_item;
                    });
                return rgb_result;
            }
            return std::nullopt;
        }

        template <typename... Args>
        static std::optional<std::variant<uint8_t, double>> ExtractRgbaColorItemIf(std::variant<Args...>&& value) {
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
            typename ArrayType, detail::EnableIf<!std::is_lvalue_reference_v<ArrayType> && detail::IsSameV<ArrayType, Array::value_type>> = true>
        static std::optional<Color> ExtractColorIf(ArrayType&& value) {
            std::string* string_ptr = std::get_if<std::string>(&value);
            if (string_ptr != nullptr) {
                assert(!string_ptr->empty());

                return std::move(*string_ptr);
            }

            InnerArray* rgb_ptr = std::get_if<InnerArray>(std::move(&value));
            if (rgb_ptr != nullptr) {
                assert(rgb_ptr->size() >= 3 && rgb_ptr->size() <= 4);
                std::vector<std::variant<uint8_t, double>> rgb_result(rgb_ptr->size());
                std::transform(
                    std::make_move_iterator(rgb_ptr->begin()), std::make_move_iterator(rgb_ptr->end()), rgb_result.begin(), [](auto&& arg) {
                        assert(std::holds_alternative<int>(arg) || std::holds_alternative<double>(arg));
                        auto rgb_item = ExtractRgbaColorItemIf(std::move(arg));
                        if (!rgb_item.has_value()) {
                            throw std::invalid_argument("Invalid RGB(a) color item");
                        }
                        return rgb_item.value();
                    });
                return rgb_result;
            }
            return std::nullopt;
        }
    };

    using RequestBase = std::unordered_map<std::string, RequestValueType>;
}

namespace transport_catalogue::io /* Requests field enums */ {
    enum class RequestType : int8_t { BASE, STAT, RENDER_SETTINGS, UNKNOWN };

    enum class RequestCommand : uint8_t { STOP, BUS, MAP, SET_SETTINGS, UNKNOWN };

    struct RequestFields {
        inline static const std::string BASE_REQUESTS{"base_requests"};
        inline static const std::string STAT_REQUESTS{"stat_requests"};
        inline static const std::string RENDER_SETTINGS{"render_settings"};
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
        inline static const std::string ID = StatRequestFields::ID;
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

    public:
        RawRequest(const RawRequest& other) = default;
        RawRequest& operator=(const RawRequest& other) = default;
        RawRequest(RawRequest&& other) = default;
        RawRequest& operator=(RawRequest&& other) = default;

        template <
            typename ReturnType, detail::EnableIf<detail::IsConvertibleV<ReturnType, ValueType> && !detail::IsSameV<ReturnType, ValueType>> = true>
        static ReturnType& Get(const ValueType& v) noexcept {
            assert(std::holds_alternative<ReturnType>(v));
            return std::get<ReturnType>(v);
        }

        template <
            typename ReturnType, detail::EnableIf<detail::IsConvertibleV<ReturnType, ValueType> && !detail::IsSameV<ReturnType, ValueType>> = true>
        static ReturnType& Get(ValueType&& v) noexcept {
            assert(std::holds_alternative<ReturnType>(v));
            return std::get<ReturnType>(std::move(v));
        }

        template <
            typename ReturnType, detail::EnableIf<detail::IsConvertibleV<ReturnType, ValueType> && !detail::IsSameV<ReturnType, ValueType>> = true>
        static std::optional<ReturnType> GetIf(const ValueType& v) noexcept {
            auto* result_ptr = std::get_if<ReturnType>(&v);
            return result_ptr ? std::optional<ReturnType>(*result_ptr) : std::nullopt;
        }

        template <
            typename ReturnType, detail::EnableIf<detail::IsConvertibleV<ReturnType, ValueType> && !detail::IsSameV<ReturnType, ValueType>> = true>
        static std::optional<ReturnType> GetIf(ValueType&& v) noexcept {
            auto* result_ptr = std::get_if<ReturnType>(std::move(&v));
            return result_ptr ? std::optional<ReturnType>{std::move(*result_ptr)} : std::nullopt;
        }

        template <
            typename ReturnType, typename KeyType,
            detail::EnableIf<detail::IsConvertibleV<ReturnType, ValueType> && !detail::IsSameV<ReturnType, ValueType>> = true>
        ReturnType* MoveIf(KeyType&& key) noexcept {
            auto ptr = find(std::forward<KeyType>(key));
            if (ptr == end()) {
                return nullptr;
            }
            auto* result_ptr = std::get_if<ReturnType>(std::move(&ptr->second));
            return result_ptr ? std::move(result_ptr) : nullptr;
        }

        template <
            typename ReturnType = ValueType, typename KeyType,
            detail::EnableIf<detail::IsConvertibleV<KeyType, RawRequest::KeyType> && detail::IsSameV<ReturnType, ValueType>> = true>
        ValueType Extract(KeyType&& key) {
            auto it = find(std::forward<KeyType>(key));

            if (it == end()) {
                return NullValue();
            }

            return std::move(extract(it).mapped());
        }

        template <
            typename ReturnType, typename KeyType,
            detail::EnableIf<detail::IsConvertibleV<ReturnType, ValueType> && !detail::IsSameV<ReturnType, ValueType>> = true>
        std::optional<ReturnType> ExtractIf(KeyType&& key) noexcept {
            auto ptr = find(std::forward<KeyType>(key));

            if (ptr == end()) {
                return std::nullopt;
            }

            assert(std::holds_alternative<ReturnType>(ptr->second));

            auto mapped = std::move(extract(ptr).mapped());
            auto* result_ptr = std::get_if<ReturnType>(std::move(&mapped));
            return result_ptr ? std::optional<ReturnType>(std::move(*result_ptr)) : std::nullopt;
        }

        template <
            typename ReturnType, typename KeyType,
            detail::EnableIf<detail::IsConvertibleV<ReturnType, ValueType> && !detail::IsSameV<ReturnType, ValueType>> = true>
        ReturnType Extract(KeyType&& key) noexcept {
            auto ptr = find(std::forward<KeyType>(key));

            if (ptr == end()) {
                return {};
            }

            assert(std::holds_alternative<ReturnType>(ptr->second));

            return std::get<ReturnType>(std::move(extract(ptr).mapped()));
        }

        template <
            typename ReturnType, typename KeyType,
            detail::EnableIf<detail::IsConvertibleV<ReturnType, ValueType> && !detail::IsSameV<ReturnType, ValueType>> = true>
        ReturnType ExtractWithThrow(KeyType&& key) {
            assert(std::holds_alternative<ReturnType>(at(key)));
            return std::get<ReturnType>(std::move(extract(key).mapped()));
        }

        template <
            typename ReturnType, typename KeyType,
            detail::EnableIf<detail::IsConvertibleV<ReturnType, ValueType> && !detail::IsSameV<ReturnType, ValueType>> = true>
        ReturnType* GetIf(KeyType&& key) noexcept {
            auto ptr = find(std::forward<KeyType>(key));
            if (ptr == end()) {
                return nullptr;
            }
            auto* result_ptr = std::get_if<ReturnType>(ptr.second);
            return result_ptr;
        }

        template <typename KeyType>
        std::optional<double> ExtractNumberValueIf(KeyType&& key) {
            auto it = find(std::forward<KeyType>(key));

            if (it == end()) {
                return std::nullopt;
            }

            ValueType value = std::move(extract(it).mapped());

            const double* double_ptr = std::get_if<double>(&value);
            if (double_ptr != nullptr) {
                return *double_ptr;
            }

            const int* int_ptr = std::get_if<int>(&value);
            if (int_ptr != nullptr) {
                return *int_ptr;
            }
            return std::nullopt;
        }

        template <typename KeyType>
        std::optional<Color> ExtractColorValueIf(KeyType&& key) {
            auto it = find(std::forward<KeyType>(key));

            if (it == end()) {
                return std::nullopt;
            }

            ValueType value = std::move(extract(it).mapped());

            /*
            std::string* string_ptr = std::get_if<std::string>(&value);
            if (string_ptr != nullptr) {
                assert(!string_ptr->empty());

                return std::move(*string_ptr);
            }

            Array* rgb_ptr = std::get_if<Array>(std::move(&value));
            if (rgb_ptr != nullptr) {
                assert(rgb_ptr->size() >= 3 && rgb_ptr->size() <= 4);
                std::vector<std::variant<uint8_t, double>> rgb_result(rgb_ptr->size());
                std::transform(
                    std::make_move_iterator(rgb_ptr->begin()), std::make_move_iterator(rgb_ptr->end()), rgb_result.begin(), [](auto&& arg) {
                        assert(std::holds_alternative<int>(arg) || std::holds_alternative<double>(arg));
                        auto rgb_item = std::visit(
                            [](auto&& arg) -> std::variant<uint8_t, double> {
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
                                    throw std::invalid_argument("Invalid Rgb[a] item");
                                }
                            },
                            arg);
                        // assert(rgb_item >= 0 && rgb_item <= std::numeric_limits<uint8_t>::max());
                        return rgb_item;
                    });
                return rgb_result;
            }
            return std::nullopt;
            */

            auto color = value.ExtractColorIf();
            return color;
        }

        template <typename KeyType>
        std::optional<std::vector<Color>> ExtractColorPaletteIf(KeyType&& key) {
            std::optional<std::vector<Color>> result = std::nullopt;

            auto array = ExtractIf<Array>(std::forward<KeyType>(key));
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

        template <typename Filter = ValueType, typename... Args>
        static ValueType VariantCast(std::variant<Args...>&& value) {
            return std::visit(
                [](auto&& arg) -> ValueType {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (detail::IsConvertibleV<T, Filter>) {
                        return std::move(arg);
                    } else {
                        return NullValue();
                    }
                },
                value);
        }

        template <typename Filter = Array, typename... Args>
        static std::optional<Array> CastToArray(std::variant<Args...>&& value) {
            return std::visit(
                [](auto&& arg) -> std::optional<Array> {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (detail::IsConvertibleV<T, Filter>) {
                        return std::move(arg);
                    } else {
                        return std::nullopt;
                    }
                },
                value);
        }
    };
}

namespace transport_catalogue::io /* Requests */ {

    struct Request {
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

        bool IsConvertedToRoundtrip() const {
            return is_converted_to_roundtrip_;
        }

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
        bool is_converted_to_roundtrip_ = false;

    private:
        void FillBus();

        void FillStop();

        void FillStops();

        void FillRoundtrip();

        void FillCoordinates();

        void FillRoadDistances();
    };

    class IStatRequest {
    public:
        virtual const std::optional<int>& GetRequestId() const = 0;
        virtual std::optional<int>& GetRequestId() = 0;
        virtual ~IStatRequest() {}
    };

    class StatRequest : public Request, public IStatRequest {
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

        const std::optional<int>& GetRequestId() const override;

        std::optional<int>& GetRequestId() override;

    protected:
        void Build() override;

    private:
        std::optional<int> request_id_;
    };

    class RenderSettingsRequest : public Request /*, public IStatRequest*/ {
    public:
        using Color = RawRequest::Color;

    public:
        RenderSettingsRequest(RequestCommand type, std::string&& name, RequestArgsMap&& args)
            : Request(std::move(type), std::move(name), std::move(args)) {
            Build();
        }
        explicit RenderSettingsRequest(RawRequest&& raw_request) : RenderSettingsRequest(RequestCommand::SET_SETTINGS, "", std::move(raw_request)) {
            Build();
        }

        bool IsBaseRequest() const override {
            return false;
        }

        bool IsStatRequest() const override {
            return false;
        }

        bool IsValidRequest() const override {
            throw std::runtime_error("Not implemented");
        }

    protected:
        void Build() override {
            width_ = args_.ExtractNumberValueIf(RenderSettingsRequestFields::WIDTH);
            height_ = args_.ExtractNumberValueIf(RenderSettingsRequestFields::HEIGHT);
            padding_ = args_.ExtractNumberValueIf(RenderSettingsRequestFields::PADDING);
            stop_radius_ = args_.ExtractNumberValueIf(RenderSettingsRequestFields::STOP_RADIUS);
            line_width_ = args_.ExtractNumberValueIf(RenderSettingsRequestFields::LINE_WIDTH);
            bus_label_font_size_ = args_.ExtractIf<int>(RenderSettingsRequestFields::BUS_LABEL_FONT_SIZE);
            bus_label_offset_ = args_.ExtractNumberValueIf(RenderSettingsRequestFields::BUS_LABEL_OFFSET);
            bus_label_font_size_ = args_.ExtractIf<int>(RenderSettingsRequestFields::STOP_LABEL_FONT_SIZE);
            stop_label_offset_ = args_.ExtractNumberValueIf(RenderSettingsRequestFields::STOP_LABEL_OFFSET);
            stop_label_offset_ = args_.ExtractNumberValueIf(RenderSettingsRequestFields::UNDERLAYER_WIDTH);
            underlayer_color_ = args_.ExtractColorValueIf(RenderSettingsRequestFields::UNDERLAYER_COLOR);
            color_palette_ = args_.ExtractColorPaletteIf(RenderSettingsRequestFields::COLOR_PALETTE);
            /*{
                //! Not implemented yet
                auto color_palette_ptr = args_.find(RenderSettingsRequestFields::COLOR_PALETTE);
                color_palette_ =
                    color_palette_ptr != args_.end() ? (assert(std::holds_alternative<Array>(const variant<Types...> &v) color_palette_ptr)):
            std::nullopt;
            }*/
        }

    private:
        std::optional<double> width_;
        std::optional<double> height_;
        std::optional<double> padding_;
        std::optional<double> stop_radius_;
        std::optional<double> line_width_;

        std::optional<int> bus_label_font_size_;
        std::optional<double> bus_label_offset_;

        std::optional<int> stop_label_font_size_;
        std::optional<double> stop_label_offset_;

        std::optional<double> underlayer_width_;
        std::optional<Color> underlayer_color_;

        std::optional<std::vector<Color>> color_palette_;
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
