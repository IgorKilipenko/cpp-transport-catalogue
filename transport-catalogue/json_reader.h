#pragma once

/*
 * Здесь можно разместить код наполнения транспортного справочника данными из JSON,
 * а также код обработки запросов к базе и формирование массива ответов в формате JSON
 */

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iterator>
#include <optional>
#include <sstream>
#include <type_traits>
#include <utility>
#include <vector>

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

namespace transport_catalogue::detail::convertors {
    template <class... Args>
    struct variant_cast_proxy {
        std::variant<Args...> v;

        template <class... ToArgs>
        operator std::variant<ToArgs...>() const {
            return std::visit(
                [](auto&& arg) -> std::variant<ToArgs...> {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (detail::IsConvertibleV<T, std::variant<ToArgs...>>) {
                        return std::move(arg);
                    } else {
                        return std::monostate();
                    }
                },
                v);
        }
    };

    template <class... Args>
    auto variant_cast(std::variant<Args...>&& v) -> variant_cast_proxy<Args...> {
        return {v};
    }
}

namespace transport_catalogue::io {

    inline Request ToRequest(json::Dict map) {
        Request result;
        std::for_each(std::make_move_iterator(map.begin()), std::make_move_iterator(map.end()), [&result](auto map_item) {
            std::string key = std::move(map_item.first);
            assert(result.count(key) == 0);
            auto node_val = map_item.second;
            if (node_val.IsArray()) {
                json::Array array = node_val.ExtractArray();
                std::vector<RequestArrayValueType> sub_array;
                sub_array.reserve(array.size());
                std::for_each(std::make_move_iterator(array.begin()), std::make_move_iterator(array.end()), [&](auto&& node) {
                    RequestArrayValueType value = detail::convertors::variant_cast(node.ExtractValue());
                    sub_array.emplace_back(std::move(value));
                });
                result.emplace(std::move(key), std::move(sub_array));
            } else {
                RequestValueType value = detail::convertors::variant_cast(node_val.ExtractValue());
                result.emplace(std::move(key), std::move(value));
            }
        });
        return result;
    }
    inline std::optional<Request> ToRequest(json::Node&& node) {
        if (!node.IsMap()) {
            return std::nullopt;
        }

        Request result;

        const json::Dict map = node.ExtractMap();
        std::for_each(std::make_move_iterator(map.begin()), std::make_move_iterator(map.end()), [&result](auto&& map_item) {
            if constexpr (std::is_convertible_v<std::decay_t<decltype(map_item.second)>, RequestValueType>) {
                result.emplace(std::move(map_item.first), std::move(map_item.second));
            }
        });

        return result;
    }
}

namespace transport_catalogue::io {
    class FileReader {
    public:
        template <typename String = std::string>
        static std::string Read(String&& filename) {
            std::ifstream istrm;
            istrm.open(filename, std::ios::in);
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

namespace transport_catalogue::io {
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
        JsonReader(std::istream& input_stream) noexcept : input_stream_{input_stream} {}

        void SetObserver(IRequestObserver* observer) override {
            observer_ = observer;
        }

        void NotifyBaseRequest(std::vector<Request>&& requests) const override {
            if (!HasObserver()) {
                return;
            }
            observer_->OnBaseRequest(std::move(requests));
        }

        void NotifyStatRequest(std::vector<Request>&& requests) const override {
            if (!HasObserver()) {
                return;
            }
            observer_->OnBaseRequest(std::move(requests));
        }

        bool HasObserver() const override {
            return observer_ != nullptr;
        }

        void ReadDocument() {
            const auto extract = [](json::Dict::ValueType* req_ptr) {
                auto array = req_ptr->ExtractArray();
                std::vector<Request> requests;
                requests.reserve(array.size());
                std::for_each(std::make_move_iterator(array.begin()), std::make_move_iterator(array.end()), [&requests](json::Node&& node) {
                    requests.push_back(ToRequest(node.ExtractMap()));
                });
                return requests;
            };
            json::Document doc = json::Document::Load(input_stream_);
            json::Node& root = doc.GetRoot();
            assert(root.IsMap());
            json::Dict raw_requests = root.ExtractMap();
            auto* base_req_ptr = raw_requests.Find(BASE_REQUESTS_LITERAL);
            auto* stat_req_ptr = raw_requests.Find(STAT_REQUESTS_LITERAL);
            assert(base_req_ptr != nullptr || stat_req_ptr != nullptr);

            if (base_req_ptr != nullptr && base_req_ptr->IsArray()) {
                // NotifyBaseRequest(ToRequest(json::Dict(base_req_ptr->AsMap())));
                std::vector<Request> requests = extract(base_req_ptr);
                NotifyBaseRequest(std::move(requests));
            }
            if (stat_req_ptr != nullptr && stat_req_ptr->IsMap()) {
                std::vector<Request> requests = extract(base_req_ptr);
                NotifyBaseRequest(std::move(requests));
            }
        }

    private:
        std::istream& input_stream_;
        IRequestObserver* observer_ = nullptr;
        constexpr static const std::string_view BASE_REQUESTS_LITERAL = "base_requests"sv;
        constexpr static const std::string_view STAT_REQUESTS_LITERAL = "stat_requests"sv;
    };
}