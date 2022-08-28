/*
 * Здесь можно было бы разместить код обработчика запросов к базе, содержащего логику, которую не
 * хотелось бы помещать ни в transport_catalogue, ни в json reader.
 *
 * Если вы затрудняетесь выбрать, что можно было бы поместить в этот файл,
 * можете оставить его пустым.
 */

#include "request_handler.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <optional>
#include <vector>

#include "domain.h"

namespace transport_catalogue::io /* RequestValueType implementation */ {

    bool RequestValueType::IsArray() const {
        return std::holds_alternative<Array>(*this);
    }

    bool RequestValueType::IsDictionary() const {
        return std::holds_alternative<std::unordered_map<std::string, RequestDictValueType>>(*this);
    }

    std::optional<RequestValueType::Color> RequestValueType::ExtractColorIf() {
        return RequestValueType::ExtractColorIf(std::move(*this));
    }
}

namespace transport_catalogue::io /* BaseRequest implementation */ {
    const std::vector<std::string>& BaseRequest::GetStops() const {
        return stops_;
    }

    std::vector<std::string>& BaseRequest::GetStops() {
        return stops_;
    }

    bool BaseRequest::IsRoundtrip() const {
        return is_roundtrip_.value_or(false);
    }

    const std::optional<data::Coordinates>& BaseRequest::GetCoordinates() const {
        return coordinates_;
    }

    std::optional<data::Coordinates>& BaseRequest::GetCoordinates() {
        return coordinates_;
    }

    const std::vector<data::MeasuredRoadDistance>& BaseRequest::GetroadDistances() const {
        return road_distances_;
    }

    std::vector<data::MeasuredRoadDistance>& BaseRequest::GetroadDistances() {
        return road_distances_;
    }

    bool BaseRequest::IsBaseRequest() const {
        return true;
    }

    bool BaseRequest::IsValidRequest() const {
        return Request::IsValidRequest() && ((IsBusCommand() && is_roundtrip_.has_value()) || (IsStopCommand() && coordinates_.has_value()));
    }

    void BaseRequest::ConvertToRoundtrip() {
        if (!is_roundtrip_.has_value() || is_roundtrip_.value()) {
            return;
        }

        ConvertToRoundtrip(stops_);

        is_roundtrip_ = true;
        is_converted_to_roundtrip_ = true;
    }

    void BaseRequest::ConvertToRoundtrip(std::vector<std::string>& stops) {
        if (stops.size() <= 1) {
            return;
        }

        size_t old_size = stops.size();
        stops.resize(old_size * 2 - 1);
        std::copy(stops.begin(), stops.begin() + old_size - 1, stops.rbegin());
    }

    void BaseRequest::Build() {
        assert(!name_.empty());
        assert(!args_.empty());
        assert(command_ == RequestCommand::BUS || command_ == RequestCommand::STOP);

        if (command_ == RequestCommand::BUS) {
            FillBus();
        } else {
            FillStop();
        }
    }

    void BaseRequest::FillBus() {
        FillStops();
        FillRoundtrip();
    }

    void BaseRequest::FillStop() {
        FillCoordinates();
        FillRoadDistances();
    }

    void BaseRequest::FillStops() {
        assert(stops_.empty());
        auto stops_tmp = args_.ExtractIf<Array>(BaseRequestFields::STOPS);

        if (!stops_tmp.has_value()) {
            return;
        }

        stops_.resize(stops_tmp.value().size());
        std::transform(
            std::make_move_iterator(stops_tmp.value().begin()), std::make_move_iterator(stops_tmp.value().end()), stops_.begin(),
            [](auto&& stop_name) {
                assert(std::holds_alternative<std::string>(stop_name));
                return std::get<std::string>(std::move(stop_name));
            });
    }

    void BaseRequest::FillRoundtrip() {
        is_roundtrip_ = args_.ExtractIf<bool>(BaseRequestFields::IS_ROUNDTRIP);
    }

    void BaseRequest::FillCoordinates() {
        std::optional<double> latitude = args_.ExtractIf<double>(BaseRequestFields::LATITUDE);
        std::optional<double> longitude = args_.ExtractIf<double>(BaseRequestFields::LONGITUDE);

        assert((latitude.has_value() && longitude.has_value()) || !(latitude.has_value() && longitude.has_value()));
        coordinates_ = !latitude.has_value() ? std::nullopt : std::optional<Coordinates>({latitude.value(), longitude.value()});
        assert(coordinates_.has_value());
    }

    void BaseRequest::FillRoadDistances() {
        auto road_distances_tmp = args_.ExtractIf<Dict>(BaseRequestFields::ROAD_DISTANCES);

        if (road_distances_tmp.has_value() && !road_distances_tmp.value().empty()) {
            for (auto&& item : road_distances_tmp.value()) {
                assert(std::holds_alternative<int>(item.second) || std::holds_alternative<double>(item.second));
                std::string to_stop = std::move(item.first);
                double distance =
                    std::holds_alternative<int>(item.second) ? std::get<int>(std::move(item.second)) : std::holds_alternative<double>(item.second);
                road_distances_.emplace_back(std::string(name_), std::move(to_stop), std::move(distance));
            }
        }
    }
}

namespace transport_catalogue::io /* Request implementation */ {

    Request::Request(std::string&& type, std::string&& name, RequestArgsMap&& args)
        : Request(
              (assert(type == converter(RequestCommand::BUS) || type == converter(RequestCommand::STOP) || type == converter(RequestCommand::MAP)),
               converter.operator()<RequestCommand>(std::move(type))),
              std::move(name), std::move(args)) {}

    Request::Request(RawRequest&& raw_request)
        /*: Request(
              (assert(raw_request.count("type") && std::holds_alternative<std::string>(raw_request.at("type"))),
               std::get<std::string>(std::move(raw_request.extract("type").mapped()))),
              (assert(raw_request.count("name")), std::get<std::string>(std::move(raw_request.extract("name").mapped()))),
              ((assert(raw_request.size() > 0), std::move(raw_request)))) {}*/
        : Request(
              (assert(raw_request.count(RequestFields::TYPE) && std::holds_alternative<std::string>(raw_request.at(RequestFields::TYPE))),
               std::get<std::string>(std::move(raw_request.extract(RequestFields::TYPE).mapped()))),
              raw_request.count(RequestFields::NAME) && std::holds_alternative<std::string>(raw_request.at(RequestFields::NAME))
                  ? std::get<std::string>(std::move(raw_request.extract(RequestFields::NAME).mapped()))
                  : "",
              ((assert(raw_request.size() > 0), std::move(raw_request)))) {}

    bool Request::IsBaseRequest() const {
        return false;
    }

    bool Request::IsStatRequest() const {
        return false;
    }

    bool Request::IsRenderSettingsRequest() const {
        return false;
    }

    bool Request::IsValidRequest() const {
        return (IsBusCommand() || IsStopCommand() || IsMapCommand() || IsRenderSettingsRequest()) && !name_.empty();
    }

    RequestCommand& Request::GetCommand() {
        return command_;
    }

    const RequestCommand& Request::GetCommand() const {
        return command_;
    }

    std::string& Request::GetName() {
        return name_;
    }

    const std::string& Request::GetName() const {
        return name_;
    }
}

namespace transport_catalogue::io /* RequestEnumConverter implementation */ {
    template <>
    std::string_view RequestEnumConverter::operator()(io::RequestCommand enum_value) const {
        using namespace std::string_view_literals;

        switch (enum_value) {
        case io::RequestCommand::BUS:
            return "Bus"sv;
        case io::RequestCommand::STOP:
            return "Stop"sv;
        case io::RequestCommand::MAP:
            return "Map"sv;
        case io::RequestCommand::UNKNOWN:
            return "Unknown"sv;
        default:
            return InvalidValue;
        }
    }

    template <>
    io::RequestCommand RequestEnumConverter::operator()(std::string_view enum_name) const {
        using namespace std::string_view_literals;

        if (enum_name == "Bus"sv) {
            return io::RequestCommand::BUS;
        } else if (enum_name == "Stop"sv) {
            return io::RequestCommand::STOP;
        } else if (enum_name == "Map"sv) {
            return io::RequestCommand::MAP;
        } else if (enum_name == "SetSettings"sv) {
            return io::RequestCommand::SET_SETTINGS;

        } else if (enum_name == "Unknown"sv) {
            return io::RequestCommand::UNKNOWN;
        }
        throw std::invalid_argument(InvalidValue);
    }

    template <>
    std::string_view RequestEnumConverter::operator()(io::RequestType enum_value) const {
        using namespace std::string_view_literals;

        switch (enum_value) {
        case io::RequestType::BASE:
            return RequestFields::BASE_REQUESTS;
        case io::RequestType::STAT:
            return RequestFields::STAT_REQUESTS;
        case io::RequestType::RENDER_SETTINGS:
            return RequestFields::RENDER_SETTINGS;
        case io::RequestType::UNKNOWN:
            return "Unknown"sv;
        default:
            return InvalidValue;
        }
    }

    template <>
    io::RequestType RequestEnumConverter::operator()(std::string_view enum_name) const {
        using namespace std::string_view_literals;

        if (enum_name == RequestFields::BASE_REQUESTS) {
            return io::RequestType::BASE;
        } else if (enum_name == RequestFields::STAT_REQUESTS) {
            return io::RequestType::STAT;
        } else if (enum_name == RequestFields::RENDER_SETTINGS) {
            return io::RequestType::RENDER_SETTINGS;
        } else if (enum_name == "Unknown"sv) {
            return io::RequestType::UNKNOWN;
        }
        throw std::invalid_argument(InvalidValue);
    }
}

namespace transport_catalogue::io /* RequestHandler implementation */ {
    /*std::optional<data::BusStat> RequestHandler::GetBusStat(const std::string_view bus_name) const {
        return db_reader_.GetBusInfo(bus_name);
    }

    const data::BusRecordSet& RequestHandler::GetBusesByStop(const std::string_view& stop_name) const {
        return db_reader_.GetDataReader().GetBuses(stop_name);
    }*/

    void RequestHandler::OnBaseRequest(std::vector<RawRequest>&& requests) const {
        std::vector<BaseRequest> reqs;
        reqs.reserve(requests.size());

        std::for_each(std::make_move_iterator(requests.begin()), std::make_move_iterator(requests.end()), [&reqs](RawRequest&& raw_req) {
            BaseRequest base_req(std::move(raw_req));
            reqs.emplace_back(std::move(base_req));
        });

        std::sort(reqs.begin(), reqs.end(), [](const BaseRequest& lhs, const BaseRequest& rhs) {
            assert(lhs.GetCommand() != RequestCommand::UNKNOWN && rhs.GetCommand() != RequestCommand::UNKNOWN);
            return static_cast<uint8_t>(lhs.GetCommand()) < static_cast<uint8_t>(rhs.GetCommand());
        });

        ExecuteRequest(std::move(reqs));
    }

    void RequestHandler::OnStatRequest(std::vector<RawRequest>&& requests) const {
        std::vector<StatRequest> reqs;
        reqs.reserve(requests.size());

        std::for_each(std::make_move_iterator(requests.begin()), std::make_move_iterator(requests.end()), [&reqs](RawRequest&& raw_req) {
            StatRequest stat_req(std::move(raw_req));
            reqs.emplace_back(std::move(stat_req));
        });

        ExecuteRequest(std::move(reqs));
    }

    void RequestHandler::OnRenderSettingsRequest(RawRequest&& request) const {
        [[maybe_unused]] RenderSettingsRequest reqs(std::move(request));
        std::cerr << "" << std::endl;
    }

    void RequestHandler::ExecuteRequest(BaseRequest&& raw_req, std::vector<data::MeasuredRoadDistance>& out_distances) const {
        assert(raw_req.IsValidRequest());

        if (raw_req.IsStopCommand()) {
            db_writer_.AddStop(data::Stop{std::move(raw_req.GetName()), std::move(raw_req.GetCoordinates().value())});
            std::move(raw_req.GetroadDistances().begin(), raw_req.GetroadDistances().end(), std::back_inserter(out_distances));

        } else {
            bool is_roundtrip = raw_req.IsRoundtrip();
            raw_req.ConvertToRoundtrip();
            db_writer_.AddBus(std::move(raw_req.GetName()), std::move(raw_req.GetStops()), is_roundtrip);
        }
    }

    void RequestHandler::ExecuteRequest(std::vector<BaseRequest>&& base_req) const {
        std::vector<data::MeasuredRoadDistance> out_distances;
        out_distances.reserve(base_req.size());

        std::for_each(std::make_move_iterator(base_req.begin()), std::make_move_iterator(base_req.end()), [this, &out_distances](BaseRequest&& req) {
            ExecuteRequest(std::move(req), out_distances);
        });

        std::for_each(
            std::make_move_iterator(out_distances.begin()), std::make_move_iterator(out_distances.end()), [this](data::MeasuredRoadDistance&& dist) {
                db_writer_.SetMeasuredDistance(std::move(dist));
            });

        out_distances.shrink_to_fit();
    }

    void RequestHandler::ExecuteRequest(StatRequest&& request) const {
        ExecuteRequest(std::vector<StatRequest>{std::move(request)});
    }

    void RequestHandler::ExecuteRequest(std::vector<StatRequest>&& requests) const {
        std::vector<StatResponse> responses;
        responses.reserve(requests.size());

        std::for_each(std::make_move_iterator(requests.begin()), std::make_move_iterator(requests.end()), [this, &responses](StatRequest&& request) {
            assert(request.IsStatRequest());
            assert(request.IsValidRequest());

            bool is_bus = request.IsBusCommand();
            bool is_stop = request.IsStopCommand();
            std::string name = request.GetName();

            StatResponse resp(
                std::move(request), is_bus ? db_reader_.GetBusInfo(name) : std::nullopt, is_stop ? db_reader_.GetStopInfo(name) : std::nullopt);

            responses.emplace_back(std::move(resp));
        });

        response_sender_.Send(std::move(responses));
    }

    void RequestHandler::SendStatResponse(StatResponse&& response) const {
        response_sender_.Send(std::move(response));
    }

    void RequestHandler::SendStatResponse(std::vector<StatResponse>&& responses) const {
        response_sender_.Send(std::move(responses));
    }
}

namespace transport_catalogue::io /* StatRequest implementation */ {
    bool StatRequest::IsBaseRequest() const {
        return false;
    }

    bool StatRequest::IsStatRequest() const {
        return true;
    }

    bool StatRequest::IsValidRequest() const {
        return Request::IsValidRequest() && request_id_.has_value();
    }

    const std::optional<int>& StatRequest::GetRequestId() const {
        return request_id_;
    }

    std::optional<int>& StatRequest::GetRequestId() {
        return request_id_;
    }

    void StatRequest::Build() {
        if (name_.empty()) {
            name_ = "TransportLayer";
        }
        request_id_ = args_.ExtractIf<int>(StatRequestFields::ID);
    }
}

namespace transport_catalogue::io /* Response implementation */ {
    int& Response::GetRequestId() {
        return request_id_;
    }

    int Response::GetRequestId() const {
        return request_id_;
    }

    bool Response::IsBusResponse() const {
        return command_ == RequestCommand::BUS;
    }

    bool Response::IsStopResponse() const {
        return command_ == RequestCommand::STOP;
    }

    bool Response::IsMapResponse() const {
        return command_ == RequestCommand::MAP;
    }

    bool Response::IsStatResponse() const {
        return false;
    }

    bool Response::IsBaseResponse() const {
        return false;
    }
}

namespace transport_catalogue::io /* StatResponse implementation */ {
    StatResponse::StatResponse(
        int&& request_id, RequestCommand&& command, std::string&& name, std::optional<data::BusStat>&& bus_stat,
        std::optional<data::StopStat>&& stop_stat)
        : Response(std::move(request_id), std::move(command), std::move(name)), bus_stat_{std::move(bus_stat)}, stop_stat_{std::move(stop_stat)} {}

    StatResponse::StatResponse(StatRequest&& request, std::optional<data::BusStat>&& bus_stat, std::optional<data::StopStat>&& stop_stat)
        : StatResponse(
              std::move((assert(request.IsValidRequest()), request.GetRequestId().value())), std::move(request.GetCommand()),
              std::move(request.GetName()), std::move(bus_stat), std::move(stop_stat)) {}

    std::optional<data::BusStat>& StatResponse::GetBusInfo() {
        return bus_stat_;
    }

    std::optional<data::StopStat>& StatResponse::GetStopInfo() {
        return stop_stat_;
    }

    bool StatResponse::IsStatResponse() const {
        return true;
    }
}

namespace transport_catalogue::io /* RenderSettingsRequest implementation */ {
    void RenderSettingsRequest::Build() {
        width_ = args_.ExtractNumberValueIf(RenderSettingsRequestFields::WIDTH);
        height_ = args_.ExtractNumberValueIf(RenderSettingsRequestFields::HEIGHT);
        padding_ = args_.ExtractNumberValueIf(RenderSettingsRequestFields::PADDING);
        stop_radius_ = args_.ExtractNumberValueIf(RenderSettingsRequestFields::STOP_RADIUS);
        line_width_ = args_.ExtractNumberValueIf(RenderSettingsRequestFields::LINE_WIDTH);
        bus_label_font_size_ = args_.ExtractIf<int>(RenderSettingsRequestFields::BUS_LABEL_FONT_SIZE);
        bus_label_offset_ = args_.ExtractOffestValueIf(RenderSettingsRequestFields::BUS_LABEL_OFFSET);
        bus_label_font_size_ = args_.ExtractIf<int>(RenderSettingsRequestFields::STOP_LABEL_FONT_SIZE);
        stop_label_offset_ = args_.ExtractOffestValueIf(RenderSettingsRequestFields::STOP_LABEL_OFFSET);
        underlayer_color_ = args_.ExtractColorValueIf(RenderSettingsRequestFields::UNDERLAYER_COLOR);
        underlayer_width_ = args_.ExtractNumberValueIf(RenderSettingsRequestFields::UNDERLAYER_WIDTH);
        color_palette_ = args_.ExtractColorPaletteIf(RenderSettingsRequestFields::COLOR_PALETTE);
    }
}