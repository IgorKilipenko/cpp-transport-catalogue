#pragma once

/*
 * Здесь можно разместить код наполнения транспортного справочника данными из JSON,
 * а также код обработки запросов к базе и формирование массива ответов в формате JSON
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <fstream>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <sstream>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
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
    struct VariantCastProxy {
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
    auto VariantCast(std::variant<Args...>&& v) -> VariantCastProxy<Args...> {
        return {v};
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
        enum class RequestType : u_int8_t { BASE, STAT };

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

        void AddObserver(std::shared_ptr<IRequestObserver> observer) override {
            observers_.emplace(haher_(observer.get()), observer);
        }

        void RemoveObserver(std::shared_ptr<IRequestObserver> observer) override {
            observers_.erase(haher_(observer.get()));
        }

        void NotifyBaseRequest(std::vector<RawRequest>&& requests) override {
            NotifyObservers(RequestType::BASE, std::move(requests));
        }

        void NotifyStatRequest(std::vector<RawRequest>&& requests) override {
            NotifyObservers(RequestType::STAT, std::move(requests));
        }

        bool HasObserver() const override {
            return std::any_of(observers_.begin(), observers_.end(), [](const auto& map_item) {
                return !map_item.second.expired();
            });
        }

        void NotifyObservers(RequestType type, std::vector<RawRequest>&& requests) {
            for (auto ptr = observers_.begin(); ptr != observers_.end();) {
                if (ptr->second.expired()) {
                    ptr = observers_.erase(ptr);
                }
                if (type == RequestType::BASE) {
                    ptr->second.lock()->OnBaseRequest(std::move(requests));
                } else {
                    ptr->second.lock()->OnStatRequest(std::move(requests));
                }
                ++ptr;
            }
        }

        void ReadDocument() {
            [[maybe_unused]] char ch = input_stream_.peek();
            if (input_stream_.peek() != json::Parser::Token::START_OBJ) {
                input_stream_.ignore(std::numeric_limits<std::streamsize>::max(), json::Parser::Token::START_OBJ)
                    .putback(json::Parser::Token::START_OBJ);
            }
            ch = input_stream_.peek();

            json::Document doc = json::Document::Load(input_stream_);
            json::Node& root = doc.GetRoot();
            assert(root.IsMap());
            json::Dict raw_requests = root.ExtractMap();
            auto* base_req_ptr = raw_requests.Find(BASE_REQUESTS_LITERAL);
            auto* stat_req_ptr = raw_requests.Find(STAT_REQUESTS_LITERAL);
            assert(base_req_ptr != nullptr || stat_req_ptr != nullptr);

            if (base_req_ptr != nullptr && base_req_ptr->IsArray()) {
                NotifyBaseRequest(JsonToRawRequest(base_req_ptr->ExtractArray()));
            }
            if (stat_req_ptr != nullptr && stat_req_ptr->IsArray()) {
                NotifyStatRequest(JsonToRawRequest(base_req_ptr->ExtractArray()));
            }
        }

    private:
        std::istream& input_stream_;
        std::unordered_map<size_t, std::weak_ptr<IRequestObserver>> observers_;
        std::hash<void*> haher_;
        constexpr static const std::string_view BASE_REQUESTS_LITERAL = "base_requests"sv;
        constexpr static const std::string_view STAT_REQUESTS_LITERAL = "stat_requests"sv;

    public: /* Helpers */
        static RawRequest JsonToRawRequest(json::Dict&& map) {
            RawRequest result;
            std::for_each(std::make_move_iterator(map.begin()), std::make_move_iterator(map.end()), [&result](auto&& map_item) {
                std::string key = std::move(map_item.first);
                assert(result.count(key) == 0);
                auto node_val = map_item.second;
                if (node_val.IsArray()) {
                    json::Array array = node_val.ExtractArray();
                    std::vector<RequestArrayValueType> sub_array;
                    sub_array.reserve(array.size());
                    std::for_each(std::make_move_iterator(array.begin()), std::make_move_iterator(array.end()), [&sub_array](auto&& node) {
                        RequestArrayValueType value = detail::convertors::VariantCast(node.ExtractValue());
                        sub_array.emplace_back(std::move(value));
                    });
                    result.emplace(std::move(key), std::move(sub_array));
                } else if (node_val.IsMap()) {
                    json::Dict map = node_val.ExtractMap();
                    std::unordered_map<std::string, RequestDictValueType> sub_map;
                    std::for_each(std::make_move_iterator(map.begin()), std::make_move_iterator(map.end()), [&sub_map](auto&& node) {
                        std::string key = std::move(node.first);
                        RequestArrayValueType value = detail::convertors::VariantCast(node.second.ExtractValue());
                        sub_map.emplace(std::move(key), std::move(value));
                    });
                    result.emplace(std::move(key), std::move(sub_map));
                } else {
                    RequestValueType value = detail::convertors::VariantCast(node_val.ExtractValue());
                    result.emplace(std::move(key), std::move(value));
                }
            });
            return result;
        }

        static std::vector<RawRequest> JsonToRawRequest(json::Array&& array) {
            std::vector<RawRequest> requests;
            requests.reserve(array.size());
            std::for_each(std::make_move_iterator(array.begin()), std::make_move_iterator(array.end()), [&requests](json::Node&& node) {
                requests.emplace_back(JsonToRawRequest(node.ExtractMap()));
            });
            return requests;
        }
    };
}