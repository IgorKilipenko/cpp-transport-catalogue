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
        std::unordered_map<const IRequestObserver*, std::weak_ptr<IRequestObserver>, Hasher> observers_;
        bool is_broadcast_ = true;
        constexpr static const std::string_view BASE_REQUESTS_LITERAL = "base_requests"sv;
        constexpr static const std::string_view STAT_REQUESTS_LITERAL = "stat_requests"sv;

    public: /* Helpers */
        static RawRequest JsonToRequest(json::Dict&& map, RequestType type);

        static std::vector<RawRequest> JsonToRequest(json::Array&& array, RequestType type);
    };
}