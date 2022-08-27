#pragma once

/*
 * Здесь можно разместить код наполнения транспортного справочника данными из JSON,
 * а также код обработки запросов к базе и формирование массива ответов в формате JSON
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <ostream>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "detail/type_traits.h"
#include "domain.h"
#include "json.h"
#include "request_handler.h"

namespace transport_catalogue::exceptions {
    class ReadingException : public std::runtime_error {
    public:
        template <typename String = std::string>
        ReadingException(String&& message) : std::runtime_error(std::forward<String>(message)) {}
    };
}

namespace transport_catalogue::detail::converters {

    template <class... Args>
    struct VariantCastProxy {
        std::variant<Args...> value;

        template <class... ToArgs>
        operator std::variant<ToArgs...>() const {
            return std::visit(
                [](auto&& arg) -> std::variant<ToArgs...> {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (detail::IsConvertibleV<T, std::variant<ToArgs...>>) {
                        return std::move(arg);
                    } else {
                        if constexpr (detail::IsConvertibleV<std::monostate, std::variant<ToArgs...>>) {
                            return std::monostate();
                        } else {
                            return nullptr;
                        }
                    }
                },
                value);
        }
    };

    template <class... Args>
    auto VariantCast(std::variant<Args...>&& value) -> VariantCastProxy<Args...> {
        return {std::move(value)};
    }

    template <typename ReturnType, typename Filter = ReturnType, typename... Args>
    ReturnType VariantCast(std::variant<Args...>&& value) {
        return std::visit(
            [](auto&& arg) -> ReturnType {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (detail::IsConvertibleV<T, Filter>) {
                    return std::move(arg);
                } else {
                    if constexpr (detail::IsConvertibleV<std::monostate, ReturnType>) {
                        return std::monostate();
                    } else {
                        return nullptr;
                    }
                }
            },
            value);
    }
}

namespace transport_catalogue::detail::io /* FileReader */ {
    class FileReader {
    public:
        template <typename Path = std::filesystem::path>
        static std::string Read(Path&& path) {
            std::ifstream istrm;
            istrm.open(path, std::ios::in);
            if (!istrm.is_open()) {
                throw exceptions::ReadingException("failed to open " /*+ filename*/);
            }
            std::string result;
            char ch;
            while (istrm.get(ch)) {
                result.push_back(ch);
            }

            istrm.close();
            return result;
        }
    };
}

namespace transport_catalogue::io /* JsonResponseSender */ {
    class JsonResponseSender : public IStatResponseSender {
    public:
        struct StatFields {
            inline static const std::string ERROR_MESSAGE{"error_message"};
            inline static const std::string REQUEST_ID{"request_id"};
            inline static const std::string CURVATURE{"curvature"};
            inline static const std::string ROUTE_LENGTH{"route_length"};
            inline static const std::string STOP_COUNT{"stop_count"};
            inline static const std::string UNIQUE_STOP_COUNT{"unique_stop_count"};
            inline static const std::string BUSES{"buses"};
        };

        JsonResponseSender(std::ostream& output_stream) : output_stream_(output_stream) {}

        bool Send(StatResponse&& response) const override;

        size_t Send(std::vector<StatResponse>&& responses) const override;

    private:
        std::ostream& output_stream_;

    private:
        json::Dict BuildStatMessage(StatResponse&& response) const;

        json::Document BuildStatResponse(std::vector<StatResponse>&& responses) const;
    };

}

namespace transport_catalogue::io /* JsonReader */ {
    using namespace std::literals;

    class JsonReader : public IRequestNotifier {
    public:
        class JsonToRequestConvertor {
            const int def_indent_step = 4;
            const int def_indent = 4;

        public:
            void operator()(std::nullptr_t) const;

            void operator()(bool value) const;

            void operator()(int value) const;

            void operator()(double value) const;

            template <typename String = std::string, detail::EnableIfSame<String, std::string> = true>
            void operator()(String&& value) const;

            template <typename Dict = json::Dict, detail::EnableIfSame<Dict, json::Dict> = true>
            void operator()(Dict&& dict) const;

            template <typename Array = json::Array, detail::EnableIfSame<Array, json::Array> = true>
            void operator()(Array&& array) const;
        };

    public:
        JsonReader(std::istream& input_stream, bool broadcast_mode = true) noexcept : input_stream_{input_stream}, is_broadcast_(broadcast_mode) {}

        void AddObserver(std::shared_ptr<IRequestObserver> observer) override;

        void RemoveObserver(std::shared_ptr<IRequestObserver> observer) override;

        void NotifyBaseRequest(std::vector<RawRequest>&& requests) override;

        void NotifyStatRequest(std::vector<RawRequest>&& requests) override;

        void NotifyRenderSettingsRequest(RawRequest&& requests) override {
            NotifyObservers(RequestType::RENDER_SETTINGS, std::vector<RawRequest>{std::move(requests)});
        }

        bool HasObserver() const override;

        void NotifyObservers(RequestType type, std::vector<RawRequest>&& requests);

        void ReadDocument();

    private:
        struct Hasher {
            size_t operator()(const IRequestObserver* ptr) const {
                return hasher_(ptr);
            }

        private:
            std::hash<const void*> hasher_;
        };

    private:
        std::istream& input_stream_;
        std::unordered_map<const IRequestObserver*, std::weak_ptr<const IRequestObserver>, Hasher> observers_;
        bool is_broadcast_ = true;
        constexpr static const std::string_view BASE_REQUESTS_LITERAL = "base_requests"sv;
        constexpr static const std::string_view STAT_REQUESTS_LITERAL = "stat_requests"sv;
        constexpr static const std::string_view RENDER_SETTINGS_REQUESTS_LITERAL = "render_settings"sv;

    public: /* Helpers */
        static RawRequest JsonToRequest(json::Dict&& map);

        static std::vector<RawRequest> JsonToRequest(json::Array&& array);

        static json::Array ConvertToJson(io::RawRequest::Array&& raw_array) {
            json::Array array(raw_array.size());
            std::transform(
                std::make_move_iterator(raw_array.begin()), std::make_move_iterator(raw_array.end()), array.begin(), [](auto&& val) -> json::Node {
                    if (std::holds_alternative<io::RawRequest::InnerArray>(val)) {
                        io::RawRequest::InnerArray inner_array = std::get<io::RawRequest::InnerArray>(val);
                        json::Array array(inner_array.size());
                        std::transform(inner_array.begin(), inner_array.end(), array.begin(), [](auto&& val) {
                            return detail::converters::VariantCast<json::Node>(std::move(val));
                        });
                        return array;
                    }
                    return detail::converters::VariantCast<json::Node>(std::move(val));
                });
            return array;
        }

        static json::Dict ConvertToJson(io::RawRequest&& request) {
            json::Dict dict;
            std::for_each(std::make_move_iterator(request.begin()), std::make_move_iterator(request.end()), [&dict](auto&& req_val) {
                std::string key = std::move(req_val.first);
                if (req_val.second.IsArray()) {
                    io::RawRequest::Array raw_array = std::get<io::RawRequest::Array>(std::move(req_val.second));
                    json::Array array = ConvertToJson(std::move(raw_array));
                    dict.emplace(std::move(key), std::move(array));
                } else {
                    json::Node val = detail::converters::VariantCast<json::Node>(std::move(req_val.second));
                    dict.emplace(std::move(key), std::move(val));
                }
            });

            return dict;
        }

        static json::Array ConvertToJsonArray(std::vector<io::RawRequest>&& requests) {
            json::Array array;
            std::for_each(std::make_move_iterator(requests.begin()), std::make_move_iterator(requests.end()), [&array](auto&& req) {
                array.push_back(io::JsonReader::ConvertToJson(std::move(req)));
            });
            return array;
        }

        static io::RawRequest::Array ConvertFromJsonArray(json::Array&& array) {
            if (array.empty()) {
                return {};
            }
            io::RawRequest::Array result;
            std::for_each(std::make_move_iterator(array.begin()), std::make_move_iterator(array.end()), [&result](json::Node&& node) {
                if (node.IsArray()) {
                    io::RawRequest::Array value = ConvertFromJsonArray(node.ExtractArray());
                    io::RawRequest::InnerArray subarray(value.size());
                    std::transform(
                        std::make_move_iterator(value.begin()), std::make_move_iterator(value.end()), subarray.begin(),
                        [](io::RawRequest::Array::value_type&& item) {
                            return detail::converters::VariantCast<io::RawRequest::InnerArray::value_type>(std::move(item));
                        });
                    result.emplace_back(std::move(subarray));
                } else {
                    RequestArrayValueType value = detail::converters::VariantCast(node.ExtractValue());
                    result.emplace_back(std::move(value));
                }
            });
            return result;
        }

        static io::RawRequest::Dict ConvertFromJsonDict(json::Dict&& dict) {
            io::RawRequest::Dict result;
            std::for_each(std::make_move_iterator(dict.begin()), std::make_move_iterator(dict.end()), [&result](auto&& node) {
                std::string key = std::move(node.first);
                io::RawRequest::Dict::mapped_type value =
                    detail::converters::VariantCast<io::RawRequest::Dict::mapped_type>(node.second.ExtractValue());
                result.emplace(std::move(key), std::move(value));
            });
            return result;
        }
    };
}