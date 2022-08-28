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
        class Converter;

    public: /* Constructors */
        JsonReader(std::istream& input_stream, bool broadcast_mode = true) noexcept : input_stream_{input_stream}, is_broadcast_(broadcast_mode) {}

    public: /* Notify interface methods */
        void AddObserver(std::shared_ptr<IRequestObserver> observer) override;

        void RemoveObserver(std::shared_ptr<IRequestObserver> observer) override;

        void NotifyBaseRequest(std::vector<RawRequest>&& requests) override;

        void NotifyStatRequest(std::vector<RawRequest>&& requests) override;

        void NotifyRenderSettingsRequest(RawRequest&& requests) override {
            NotifyObservers(RequestType::RENDER_SETTINGS, std::vector<RawRequest>{std::move(requests)});
        }

        bool HasObserver() const override;

    public: /* Public methods */
        void NotifyObservers(RequestType type, std::vector<RawRequest>&& requests);

        void ReadDocument();

    private: /* Inner classes */
        struct Hasher {
        public:
            size_t operator()(const IRequestObserver* ptr) const {
                return hasher_(ptr);
            }

        private:
            std::hash<const void*> hasher_;
        };

    private: /* Inner members */
        std::istream& input_stream_;
        std::unordered_map<const IRequestObserver*, std::weak_ptr<const IRequestObserver>, Hasher> observers_;
        bool is_broadcast_ = true;

    public: /* Constant values */
        constexpr static const std::string_view BASE_REQUESTS_LITERAL = "base_requests"sv;
        constexpr static const std::string_view STAT_REQUESTS_LITERAL = "stat_requests"sv;
        constexpr static const std::string_view RENDER_SETTINGS_REQUESTS_LITERAL = "render_settings"sv;
    };
}

namespace transport_catalogue::io /* JsonReader::Converter */ {

    class JsonReader::Converter {
    public:
        static RawRequest JsonToRequest(json::Dict&& map);

        static std::vector<RawRequest> JsonToRequest(json::Array&& array);

        static json::Array ConvertToJson(RawRequest::Array&& raw_array);

        static json::Dict ConvertToJson(RawRequest&& request);

        static json::Array ConvertToJsonArray(std::vector<io::RawRequest>&& requests);

        static RawRequest::Array ConvertFromJsonArray(json::Array&& array);

        static RawRequest::Dict ConvertFromJsonDict(json::Dict&& dict);
    };
}