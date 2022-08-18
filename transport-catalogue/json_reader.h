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
        return {std::move(v)};
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
        JsonReader(std::istream& input_stream, bool broadcast_mode = true) noexcept : input_stream_{input_stream}, is_broadcast_(broadcast_mode) {}

        void AddObserver(std::shared_ptr<IRequestObserver> observer) override {
            if (!is_broadcast_ && !observers_.empty()) {
                throw std::logic_error("Duplicate listener. This instance supports only one listener, (not supports broadcast mode)");
            }
            observers_.emplace(observer.get(), observer);
        }

        void RemoveObserver(std::shared_ptr<IRequestObserver> observer) override {
            observers_.erase(observer.get());
        }

        void NotifyBaseRequest(std::vector<Request>&& requests) override {
            NotifyObservers(RequestType::BASE, std::move(requests));
        }

        void NotifyStatRequest(std::vector<Request>&& requests) override {
            NotifyObservers(RequestType::STAT, std::move(requests));
        }

        bool HasObserver() const override {
            return std::any_of(observers_.begin(), observers_.end(), [](const auto& map_item) {
                return !map_item.second.expired();
            });
        }

        void NotifyObservers(RequestType type, std::vector<Request>&& requests) {
            assert(is_broadcast_ || observers_.size() == 1);

            for (auto ptr = observers_.begin(); ptr != observers_.end();) {
                if (ptr->second.expired()) {
                    ptr = observers_.erase(ptr);
                    continue;
                }
                if (type == RequestType::BASE) {
                    ptr->second.lock()->OnBaseRequest(is_broadcast_ && observers_.size() > 1 ? requests : std::move(requests));
                    [[maybe_unused]] bool jj = false;  //!!! FOR DEBUG ONLY
                } else {
                    ptr->second.lock()->OnStatRequest(is_broadcast_ && observers_.size() > 1 ? requests : std::move(requests));
                }

                ptr = is_broadcast_ ? ++ptr : observers_.end();
            }
        }

        void ReadDocument() {
            [[maybe_unused]] char ch = input_stream_.peek();  //!!!!!!!!!!! FOR DEBUG ONLY
            if (input_stream_.peek() != json::Parser::Token::START_OBJ) {
                input_stream_.ignore(std::numeric_limits<std::streamsize>::max(), json::Parser::Token::START_OBJ)
                    .putback(json::Parser::Token::START_OBJ);
            }
            ch = input_stream_.peek();  //!!!!!!!!!!! FOR DEBUG ONLY

            json::Document doc = json::Document::Load(input_stream_);
            json::Node& root = doc.GetRoot();
            assert(root.IsMap());
            json::Dict raw_requests = root.ExtractMap();
            auto base_req_ptr = std::move_iterator(raw_requests.find(BASE_REQUESTS_LITERAL));
            auto stat_req_ptr = std::move_iterator(raw_requests.find(STAT_REQUESTS_LITERAL));
            auto end = std::move_iterator(raw_requests.end());
            assert(base_req_ptr != end || stat_req_ptr != end);

            if (base_req_ptr != std::move_iterator(raw_requests.end()) && base_req_ptr->second.IsArray()) {
                json::Array array = base_req_ptr->second.ExtractArray();
                auto req = JsonToRequest(std::move(array), RequestType::BASE);
                NotifyBaseRequest(std::move(req));
            }
            if (stat_req_ptr != std::move_iterator(raw_requests.end()) && stat_req_ptr->second.IsArray()) {
                json::Array array = stat_req_ptr->second.ExtractArray();
                NotifyBaseRequest(JsonToRequest(std::move(array), RequestType::STAT));
            }
        }

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
        std::unordered_map<const IRequestObserver*, std::weak_ptr<IRequestObserver>, Hasher> observers_;
        bool is_broadcast_ = true;
        constexpr static const std::string_view BASE_REQUESTS_LITERAL = "base_requests"sv;
        constexpr static const std::string_view STAT_REQUESTS_LITERAL = "stat_requests"sv;

    public: /* Helpers */
        static Request JsonToRequest(json::Dict&& map, RequestType type) {
            RawRequest result;
            std::for_each(std::make_move_iterator(map.begin()), std::make_move_iterator(map.end()), [&result](auto&& map_item) {
                std::string key = std::move(map_item.first);
                assert(result.count(key) == 0);
                auto node_val = std::move(map_item.second);
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
            return type == RequestType::BASE ? BaseRequest(std::move(result)) : Request(std::move(result));
        }

        static std::vector<Request> JsonToRequest(json::Array&& array, RequestType type) {
            std::vector<Request> requests;
            requests.reserve(array.size());
            std::for_each(std::make_move_iterator(array.begin()), std::make_move_iterator(array.end()), [&requests, type](json::Node&& node) {
                requests.emplace_back(JsonToRequest(node.ExtractMap(), type));
            });
            return requests;
        }
    };
}