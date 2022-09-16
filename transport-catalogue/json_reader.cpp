#include "json_reader.h"

#include <algorithm>
#include <type_traits>
#include <variant>

#include "json.h"
#include "json_builder.h"

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

    void JsonReader::NotifyRenderSettingsRequest(RawRequest&& requests) {
        NotifyObservers(RequestType::RENDER_SETTINGS, std::vector<RawRequest>{std::move(requests)});
    }

    void JsonReader::NotifyRoutingSettingsRequest(RawRequest&& requests) {
        NotifyObservers(RequestType::ROUTING_SETTINGS, std::vector<RawRequest>{std::move(requests)});
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
                assert(requests.size() == 1);
                ptr->second.lock()->OnRenderSettingsRequest(is_broadcast_ && observers_.size() > 1 ? requests.front() : std::move(requests.front()));
            } else if (type == RequestType::ROUTING_SETTINGS) {
                assert(requests.size() == 1);
                ptr->second.lock()->OnRoutingSettingsRequest(is_broadcast_ && observers_.size() > 1 ? requests.front() : std::move(requests.front()));
            }

            ptr = is_broadcast_ ? ++ptr : observers_.end();
        }
    }

    void JsonReader::ReadDocument() {
        json::Document doc = json::Document::Load(input_stream_);
        json::Node& root = doc.GetRoot();

        assert(root.IsMap());

        json::Dict raw_requests = root.ExtractMap();
        auto render_settings_req_ptr = std::move_iterator(raw_requests.find(RENDER_SETTINGS_REQUESTS_LITERAL));
        auto routing_settings_req_ptr = std::move_iterator(raw_requests.find(ROUTING_SETTINGS_REQUESTS_LITERAL));
        auto base_req_ptr = std::move_iterator(raw_requests.find(BASE_REQUESTS_LITERAL));
        auto stat_req_ptr = std::move_iterator(raw_requests.find(STAT_REQUESTS_LITERAL));

        auto end = std::move_iterator(raw_requests.end());

        assert(base_req_ptr != end || stat_req_ptr != end || render_settings_req_ptr != end || routing_settings_req_ptr != end);

        //! Fill data. Must be executed before requesting statistics
        if (base_req_ptr != end && base_req_ptr->second.IsArray()) {
            json::Array array = base_req_ptr->second.ExtractArray();
            auto req = Converter::JsonToRequest(std::move(array));
            NotifyBaseRequest(std::move(req));
        }
        //! Render configuration must be done before requesting statistics
        if (render_settings_req_ptr != end && render_settings_req_ptr->second.IsMap()) {
            json::Dict dict = render_settings_req_ptr->second.ExtractMap();
            NotifyRenderSettingsRequest(Converter::JsonToRequest(std::move(dict)));
        }
        if (routing_settings_req_ptr != end && routing_settings_req_ptr->second.IsMap()) {
            json::Dict dict = routing_settings_req_ptr->second.ExtractMap();
            NotifyRoutingSettingsRequest(Converter::JsonToRequest(std::move(dict)));
        }
        if (stat_req_ptr != end && stat_req_ptr->second.IsArray()) {
            json::Array array = stat_req_ptr->second.ExtractArray();
            NotifyStatRequest(Converter::JsonToRequest(std::move(array)));
        }

        //! throw exception
    }
}

namespace transport_catalogue::io /* JsonReader::Converter implementation */ {

    RawRequest JsonReader::Converter::JsonToRequest(json::Dict&& map) {
        RawRequest result;
        std::for_each(std::make_move_iterator(map.begin()), std::make_move_iterator(map.end()), [&result](auto&& map_item) {
            std::string key = std::move(map_item.first);
            assert(result.count(key) == 0);
            auto node_val = std::move(map_item.second);
            if (node_val.IsArray()) {
                RawRequest::Array array = ConvertFromJsonArray(node_val.ExtractArray());
                result.emplace(std::move(key), std::move(array));
            } else if (node_val.IsMap()) {
                RawRequest::Dict map = ConvertFromJsonDict(node_val.ExtractMap());
                result.emplace(std::move(key), std::move(map));
            } else {
                RequestValueType value = RawRequest::VariantCast<RequestValueType::AtomicValueType>(node_val.ExtractValue());
                result.emplace(std::move(key), std::move(value));
            }
        });
        return result;
    }

    std::vector<RawRequest> JsonReader::Converter::JsonToRequest(json::Array&& array) {
        std::vector<RawRequest> requests;
        requests.reserve(array.size());
        std::for_each(std::make_move_iterator(array.begin()), std::make_move_iterator(array.end()), [&requests](json::Node&& node) {
            requests.emplace_back(JsonToRequest(node.ExtractMap()));
        });
        return requests;
    }

    json::Array JsonReader::Converter::ConvertToJson(io::RawRequest::Array&& raw_array) {
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

    json::Dict JsonReader::Converter::ConvertToJson(io::RawRequest&& request) {
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

    json::Array JsonReader::Converter::ConvertToJsonArray(std::vector<io::RawRequest>&& requests) {
        json::Array array;
        std::for_each(std::make_move_iterator(requests.begin()), std::make_move_iterator(requests.end()), [&array](auto&& req) {
            array.push_back(JsonReader::Converter::ConvertToJson(std::move(req)));
        });
        return array;
    }

    io::RawRequest::Array JsonReader::Converter::ConvertFromJsonArray(json::Array&& array) {
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

    io::RawRequest::Dict JsonReader::Converter::ConvertFromJsonDict(json::Dict&& dict) {
        io::RawRequest::Dict result;
        std::for_each(std::make_move_iterator(dict.begin()), std::make_move_iterator(dict.end()), [&result](auto&& node) {
            std::string key = std::move(node.first);
            io::RawRequest::Dict::mapped_type value = detail::converters::VariantCast<io::RawRequest::Dict::mapped_type>(node.second.ExtractValue());
            result.emplace(std::move(key), std::move(value));
        });
        return result;
    }
}

namespace transport_catalogue::io /* JsonResponseSender implementation */ {
    bool JsonResponseSender::Send(StatResponse&& response) const {
        json::Document doc = BuildStatResponse_({std::move(response)});
        doc.Print(output_stream_);
        return true;
    }

    size_t JsonResponseSender::Send(std::vector<StatResponse>&& responses) const {
        if (responses.empty()) {
            return 0;
        }
        json::Document doc = BuildStatResponse_(std::move(responses));
        doc.Print(output_stream_);
        return doc.GetRoot().AsArray().size();
    }

    json::Dict JsonResponseSender::BuildStatMessage_(StatResponse&& response) const {
        static const json::Dict::ItemType ERROR_MESSAGE_ITEM{StatFields::ERROR_MESSAGE, "not found"};
        json::Builder builder;
        auto dict_context = builder.StartDict().Key(StatFields::REQUEST_ID).Value(response.GetRequestId());
        if (response.IsBusResponse()) {
            auto stat = std::move(response.GetBusInfo());
            if (!stat.has_value()) {
                dict_context.Key(ERROR_MESSAGE_ITEM.first).Value(ERROR_MESSAGE_ITEM.second);
            } else {
                dict_context.Key(StatFields::CURVATURE)
                    .Value(stat->route_curvature)
                    .Key(StatFields::ROUTE_LENGTH)
                    .Value(static_cast<int>(stat->route_length))
                    .Key(StatFields::STOP_COUNT)
                    .Value(static_cast<int>(stat->total_stops))
                    .Key(StatFields::UNIQUE_STOP_COUNT)
                    .Value(static_cast<int>(stat->unique_stops));
            }
        } else if (response.IsStopResponse()) {
            auto stat = std::move(response.GetStopInfo());
            if (!stat.has_value()) {
                dict_context.Key(ERROR_MESSAGE_ITEM.first).Value(ERROR_MESSAGE_ITEM.second);
            } else {
                dict_context.Key(StatFields::BUSES)
                    .Value(json::Array(std::make_move_iterator(stat->buses.begin()), std::make_move_iterator(stat->buses.end())));
            }
        } else if (response.IsMapResponse()) {
            auto map = std::move(response.GetMapData());
            if (!map.has_value()) {
                dict_context.Key(ERROR_MESSAGE_ITEM.first).Value(ERROR_MESSAGE_ITEM.second);
            } else {
                dict_context.Key(StatFields::MAP).Value(map.value());
            }
        } else if (response.IsRouteResponse()) {
            auto route_info = std::move(response.GetRouteInfo());
            if (!route_info.has_value()) {
                dict_context.Key(ERROR_MESSAGE_ITEM.first).Value(ERROR_MESSAGE_ITEM.second);
            } else {
                BuildRouteMessage_(std::move(route_info.value()), dict_context);
            }
        } else {
            throw exceptions::ReadingException("Invalid response (Is not stat response). Response does not contain stat info");
        }
        dict_context.EndDict();
        json::Dict dict = std::move(builder.Extract().AsMap());
        return dict;
    }

    void JsonResponseSender::BuildRouteMessage_(RouteInfo&& route_info, json::Builder::KeyValueContext& dict_context) const {
        dict_context.Key(StatFields::TOTAL_TIME).Value(static_cast<double>(route_info.total_time));
        auto&& items = std::move(route_info.items);
        json::Array items_json;
        items_json.reserve(items.size());
        std::for_each(std::make_move_iterator(items.begin()), std::make_move_iterator(items.end()), [&items_json](auto&& item) {
            RouteInfo::WaitInfo wait_info = std::move(item.second);
            items_json.emplace_back(
                json::Dict{{"stop_name", static_cast<std::string>(wait_info.stop_name)}, {"time", static_cast<double>(wait_info.time)}});

            RouteInfo::BusInfo info = std::move(item.first);
            items_json.emplace_back(json::Dict{
                {"bus", static_cast<std::string>(info.bus)},
                {"span_count", static_cast<int>(info.span_count)},
                {"time", static_cast<double>(info.time)}});
        });
        dict_context.Key(StatFields::ITEMS).Value(std::move(items_json));
    }

    json::Document JsonResponseSender::BuildStatResponse_(std::vector<StatResponse>&& responses) const {
        json::Array json_response;
        std::for_each(std::move_iterator(responses.begin()), std::move_iterator(responses.end()), [this, &json_response](StatResponse&& response) {
            json::Dict resp_value = BuildStatMessage_(std::move(response));
            json_response.emplace_back(std::move(resp_value));
        });
        json::Document doc(std::move(json_response));
        return doc;
    }
}
