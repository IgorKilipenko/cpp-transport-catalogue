#include "json_reader.h"

/*
 * Здесь можно разместить код наполнения транспортного справочника данными из JSON,
 * а также код обработки запросов к базе и формирование массива ответов в формате JSON
 */

namespace transport_catalogue::io /* JsonReader implementation */ {

    void JsonReader::AddObserver(std::shared_ptr<IRequestObserver> observer) {
        if (!is_broadcast_ && !observers_.empty()) {
            throw std::logic_error("Duplicate listener. This instance supports only one listener, (not supports broadcast mode)");
        }
        observers_.emplace(observer.get(), observer);
    }

    void JsonReader::RemoveObserver(std::shared_ptr<IRequestObserver> observer) {
        observers_.erase(observer.get());
    }

    void JsonReader::NotifyBaseRequest(std::vector<RawRequest>&& requests) {
        NotifyObservers(RequestType::BASE, std::move(requests));
    }

    void JsonReader::NotifyStatRequest(std::vector<RawRequest>&& requests) {
        NotifyObservers(RequestType::STAT, std::move(requests));
    }

    bool JsonReader::HasObserver() const {
        return std::any_of(observers_.begin(), observers_.end(), [](const auto& map_item) {
            return !map_item.second.expired();
        });
    }

    void JsonReader::NotifyObservers(RequestType type, std::vector<RawRequest>&& requests) {
        assert(is_broadcast_ || observers_.size() == 1);

        for (auto ptr = observers_.begin(); ptr != observers_.end();) {
            if (ptr->second.expired()) {
                ptr = observers_.erase(ptr);
                continue;
            }
            if (type == RequestType::BASE) {
                ptr->second.lock()->OnBaseRequest(is_broadcast_ && observers_.size() > 1 ? requests : std::move(requests));
            } else if (type == RequestType::STAT) {
                ptr->second.lock()->OnStatRequest(is_broadcast_ && observers_.size() > 1 ? requests : std::move(requests));
            } else if (type == RequestType::RENDER_SETTINGS) {
                ptr->second.lock()->OnRenderSettingsRequest(is_broadcast_ && observers_.size() > 1 ? requests : std::move(requests));
            }

            ptr = is_broadcast_ ? ++ptr : observers_.end();
        }
    }

    void JsonReader::ReadDocument() {
        /*[[maybe_unused]] char ch = input_stream_.peek();  //!!!!!!!!!!! FOR DEBUG ONLY
        if (input_stream_.peek() != json::Parser::Token::START_OBJ) {
            input_stream_.ignore(std::numeric_limits<std::streamsize>::max(), json::Parser::Token::START_OBJ)
                .putback(json::Parser::Token::START_OBJ);
        }
        ch = input_stream_.peek();  //!!!!!!!!!!! FOR DEBUG ONLY*/

        json::Document doc = json::Document::Load(input_stream_);
        json::Node& root = doc.GetRoot();

        assert(root.IsMap());

        json::Dict raw_requests = root.ExtractMap();
        auto base_req_ptr = std::move_iterator(raw_requests.find(BASE_REQUESTS_LITERAL));
        auto stat_req_ptr = std::move_iterator(raw_requests.find(STAT_REQUESTS_LITERAL));
        auto render_settings_req_ptr = std::move_iterator(raw_requests.find(RENDER_SETTINGS_REQUESTS_LITERAL));

        auto end = std::move_iterator(raw_requests.end());

        assert(base_req_ptr != end || stat_req_ptr != end || render_settings_req_ptr != end);

        if (base_req_ptr != end && base_req_ptr->second.IsArray()) {
            json::Array array = base_req_ptr->second.ExtractArray();
            auto req = JsonToRequest(std::move(array), RequestType::BASE);
            NotifyBaseRequest(std::move(req));
        }
        if (stat_req_ptr != end && stat_req_ptr->second.IsArray()) {
            json::Array array = stat_req_ptr->second.ExtractArray();
            NotifyStatRequest(JsonToRequest(std::move(array), RequestType::STAT));
        }
        if (render_settings_req_ptr != end && render_settings_req_ptr->second.IsArray()) {
            json::Array array = render_settings_req_ptr->second.ExtractArray();
            NotifyRenderSettingsRequest(JsonToRequest(std::move(array), RequestType::RENDER_SETTINGS));
        }
    }

    RawRequest JsonReader::JsonToRequest(json::Dict&& map, RequestType type) {
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
                    RequestArrayValueType value = detail::converters::VariantCast(node.ExtractValue());
                    sub_array.emplace_back(std::move(value));
                });
                result.emplace(std::move(key), std::move(sub_array));
            } else if (node_val.IsMap()) {
                json::Dict map = node_val.ExtractMap();
                std::unordered_map<std::string, RequestDictValueType> sub_map;
                std::for_each(std::make_move_iterator(map.begin()), std::make_move_iterator(map.end()), [&sub_map](auto&& node) {
                    std::string key = std::move(node.first);
                    RequestArrayValueType value = detail::converters::VariantCast(node.second.ExtractValue());
                    sub_map.emplace(std::move(key), std::move(value));
                });
                result.emplace(std::move(key), std::move(sub_map));
            } else {
                RequestValueType value = detail::converters::VariantCast(node_val.ExtractValue());
                result.emplace(std::move(key), std::move(value));
            }
        });
        return result;
    }

    std::vector<RawRequest> JsonReader::JsonToRequest(json::Array&& array, RequestType type) {
        std::vector<RawRequest> requests;
        requests.reserve(array.size());
        std::for_each(std::make_move_iterator(array.begin()), std::make_move_iterator(array.end()), [&requests, type](json::Node&& node) {
            requests.emplace_back(JsonToRequest(node.ExtractMap(), type));
        });
        return requests;
    }
}

namespace transport_catalogue::io /* JsonResponseSender implementation */ {
    bool JsonResponseSender::Send(StatResponse&& response) const {
        json::Document doc = BuildStatResponse({std::move(response)});
        doc.Print(output_stream_);
        return true;
    }

    size_t JsonResponseSender::Send(std::vector<StatResponse>&& responses) const {
        if (responses.empty()) {
            return 0;
        }
        json::Document doc = BuildStatResponse(std::move(responses));
        doc.Print(output_stream_);
        return doc.GetRoot().AsArray().size();
    }

    json::Dict JsonResponseSender::BuildStatMessage(StatResponse&& response) const {
        static const json::Dict::ItemType ERROR_MESSAGE_ITEM{StatFields::ERROR_MESSAGE, "not found"};
        json::Dict map;
        map[StatFields::REQUEST_ID] = response.GetRequestId();
        if (response.IsBusResponse()) {
            auto stat = std::move(response.GetBusInfo());
            if (!stat.has_value()) {
                map.insert(ERROR_MESSAGE_ITEM);
            } else {
                map[StatFields::CURVATURE] = stat->route_curvature;
                map[StatFields::ROUTE_LENGTH] = static_cast<int>(stat->route_length);
                map[StatFields::STOP_COUNT] = static_cast<int>(stat->total_stops);
                map[StatFields::UNIQUE_STOP_COUNT] = static_cast<int>(stat->unique_stops);
            }
        } else if (response.IsStopResponse()) {
            auto stat = std::move(response.GetStopInfo());
            if (!stat.has_value()) {
                map.insert(ERROR_MESSAGE_ITEM);
            } else {
                map[StatFields::BUSES] = json::Array(std::make_move_iterator(stat->buses.begin()), std::make_move_iterator(stat->buses.end()));
            }
        } else {
            throw exceptions::ReadingException("Invalid response (Is not stat response). Response does not contain stat info");
        }
        return map;
    }

    json::Document JsonResponseSender::BuildStatResponse(std::vector<StatResponse>&& responses) const {
        json::Array json_response;
        std::for_each(std::move_iterator(responses.begin()), std::move_iterator(responses.end()), [this, &json_response](StatResponse&& response) {
            json::Dict resp_value = BuildStatMessage(std::move(response));
            json_response.emplace_back(std::move(resp_value));
        });
        json::Document doc(std::move(json_response));
        return doc;
    }
}
