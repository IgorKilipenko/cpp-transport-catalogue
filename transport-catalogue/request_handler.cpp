#include "request_handler.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <vector>

#include "domain.h"
#include "svg.h"  //! FOR DEBUG ONLY
#include "transport_router.h"

namespace transport_catalogue::io /* RequestValueType implementation */ {

    bool RequestValueType::IsArray() const {
        return std::holds_alternative<Array>(*this);
    }

    bool RequestValueType::IsDictionary() const {
        return std::holds_alternative<std::unordered_map<std::string, RequestDictValueType>>(*this);
    }

    std::optional<double> RequestValueType::ExtractNumericIf() {
        return ExtractNumericIf(std::move(*this));
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
        assert(!args_.empty());
        assert(command_ == RequestCommand::BUS || command_ == RequestCommand::STOP);

        name_ = name_.empty() ? args_.ExtractIf<std::string>(BaseRequestFields::NAME).value_or("") : name_;
        assert(!name_.empty());

        if (command_ == RequestCommand::BUS) {
            FillBus_();
        } else {
            FillStop_();
        }
    }

    std::string& BaseRequest::GetName() {
        return name_;
    }

    const std::string& BaseRequest::GetName() const {
        return name_;
    }

    void BaseRequest::FillBus_() {
        FillStops_();
        FillRoundtrip_();
    }

    void BaseRequest::FillStop_() {
        FillCoordinates_();
        FillRoadDistances_();
    }

    void BaseRequest::FillStops_() {
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

    void BaseRequest::FillRoundtrip_() {
        is_roundtrip_ = args_.ExtractIf<bool>(BaseRequestFields::IS_ROUNDTRIP);
    }

    void BaseRequest::FillCoordinates_() {
        std::optional<double> latitude = args_.ExtractNumberValueIf(BaseRequestFields::LATITUDE);
        std::optional<double> longitude = args_.ExtractNumberValueIf(BaseRequestFields::LONGITUDE);

        assert((latitude.has_value() && longitude.has_value()) || !(latitude.has_value() && longitude.has_value()));
        coordinates_ = !latitude.has_value() ? std::nullopt : std::optional<Coordinates>({latitude.value(), longitude.value()});
        assert(coordinates_.has_value());
    }

    void BaseRequest::FillRoadDistances_() {
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

    Request::Request(std::string&& type, RequestArgsMap&& args)
        : Request(
              (assert(
                   type == converter(RequestCommand::BUS) || type == converter(RequestCommand::STOP) || type == converter(RequestCommand::MAP) ||
                   type == converter(RequestCommand::ROUTE)),
               converter.ToRequestCommand(std::move(type))),
              std::move(args)) {}

    Request::Request(RawRequest&& raw_request)
        : Request(
              (assert(raw_request.count(RequestFields::TYPE) && std::holds_alternative<std::string>(raw_request.at(RequestFields::TYPE))),
               std::get<std::string>(std::move(raw_request.extract(RequestFields::TYPE).mapped()))),
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

    bool Request::IsRoutingSettingsRequest() const {
        return false;
    }

    bool Request::IsValidRequest() const {
        return (IsBusCommand() || IsStopCommand() || IsMapCommand() || IsRouteCommand() || IsRenderSettingsRequest() || IsRenderSettingsRequest());
    }

    RequestCommand& Request::GetCommand() {
        return command_;
    }

    const RequestCommand& Request::GetCommand() const {
        return command_;
    }
}

namespace transport_catalogue::io /* RequestEnumConverter implementation */ {

    std::string_view RequestEnumConverter::operator()(io::RequestCommand enum_value) const {
        using namespace std::string_view_literals;

        switch (enum_value) {
        case io::RequestCommand::BUS:
            return "Bus"sv;
        case io::RequestCommand::STOP:
            return "Stop"sv;
        case io::RequestCommand::MAP:
            return "Map"sv;
        case io::RequestCommand::ROUTE:
            return "Route"sv;
        case io::RequestCommand::UNKNOWN:
            return "Unknown"sv;
        default:
            return InvalidValue;
        }
    }

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

    RequestType RequestEnumConverter::ToRequestType(std::string_view enum_name) const {
        using namespace std::string_view_literals;

        if (enum_name == RequestFields::BASE_REQUESTS) {
            return RequestType::BASE;
        } else if (enum_name == RequestFields::STAT_REQUESTS) {
            return RequestType::STAT;
        } else if (enum_name == RequestFields::RENDER_SETTINGS) {
            return RequestType::RENDER_SETTINGS;
        } else if (enum_name == RequestFields::ROUTING_SETTINGS) {
            return RequestType::ROUTING_SETTINGS;
        } else if (enum_name == "Unknown"sv) {
            return RequestType::UNKNOWN;
        }
        throw std::invalid_argument(InvalidValue);
    }

    RequestCommand RequestEnumConverter::ToRequestCommand(std::string_view enum_name) const {
        using namespace std::string_view_literals;

        if (enum_name == "Bus"sv) {
            return io::RequestCommand::BUS;
        } else if (enum_name == "Stop"sv) {
            return io::RequestCommand::STOP;
        } else if (enum_name == "Map"sv) {
            return io::RequestCommand::MAP;
        } else if (enum_name == "Route"sv) {
            return io::RequestCommand::ROUTE;
        } else if (enum_name == "SetSettings"sv) {
            return io::RequestCommand::SET_RENDER_SETTINGS;
        } else if (enum_name == "SetRouteSettings"sv) {
            return io::RequestCommand::SET_ROUTING_SETTINGS;
        } else if (enum_name == "Unknown"sv) {
            return io::RequestCommand::UNKNOWN;
        }
        throw std::invalid_argument(InvalidValue);
    }
}

namespace transport_catalogue::io /* RequestHandler implementation */ {

    void RequestHandler::OnBaseRequest(std::vector<RawRequest>&& requests) {
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

    void RequestHandler::OnStatRequest(std::vector<RawRequest>&& requests) {
        std::vector<StatRequest> reqs;
        reqs.reserve(requests.size());

        std::for_each(std::make_move_iterator(requests.begin()), std::make_move_iterator(requests.end()), [&reqs](RawRequest&& raw_req) {
            StatRequest stat_req(std::move(raw_req));
            reqs.emplace_back(std::move(stat_req));
        });

        ExecuteRequest(std::move(reqs));
    }

    void RequestHandler::OnRenderSettingsRequest(RawRequest&& request) {
        RenderSettingsRequest reqs(std::move(request));
        ExecuteRequest(std::move(reqs));
    }

    void RequestHandler::OnRoutingSettingsRequest(RawRequest&& request) {
        RoutingSettingsRequest reqs(std::move(request));
        ExecuteRequest(std::move(reqs));
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

    void RequestHandler::ExecuteRequest(std::vector<BaseRequest>&& base_req) {
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

    void RequestHandler::ExecuteRequest(StatRequest&& request) {
        ExecuteRequest(std::vector<StatRequest>{std::move(request)});
    }

    void RequestHandler::ExecuteRequest(std::vector<StatRequest>&& requests) {
        std::vector<StatResponse> responses;
        responses.reserve(requests.size());

        std::for_each(std::make_move_iterator(requests.begin()), std::make_move_iterator(requests.end()), [this, &responses](StatRequest&& request) {
            assert(request.IsStatRequest());
            assert(request.IsValidRequest());

            bool is_bus = request.IsBusCommand();
            bool is_stop = request.IsStopCommand();
            bool is_map = request.IsMapCommand();
            bool is_router = request.IsRouteCommand();

            std::string name = request.GetName().value_or("");
            std::optional<RouteSataRequest> route_request =
                is_router ? std::optional<RouteSataRequest>{RouteSataRequest(StatRequest(request))} : std::nullopt;

            StatResponse resp(
                std::move(request), is_bus ? db_reader_.GetBusInfo(name) : std::nullopt, is_stop ? db_reader_.GetStopInfo(name) : std::nullopt,
                is_map ? std::optional<RawMapData>(RenderMap()) : std::nullopt,
                is_router ? std::optional<RouteInfo>(router_.GetRouteInfo(route_request->GetFromStop().value(), route_request->GetToStop().value()))
                          : std::nullopt);

            responses.emplace_back(std::move(resp));
        });

        response_sender_.Send(std::move(responses));
    }

    void RequestHandler::ExecuteRequest(RenderSettingsRequest&& request) {
        maps::RenderSettings settings = SettingsBuilder::BuildMapRenderSettings(std::move(request));
        renderer_.SetRenderSettings(std::move(settings));
    }

    void RequestHandler::ExecuteRequest(RoutingSettingsRequest&& request) {
        router_.SetSettings(
            {static_cast<double>(request.GetBusWaitTimeMin().value_or(0)), static_cast<double>(request.GetBusVelocityKmh().value_or(0))});
        router_.Build();
    }

    void RequestHandler::SendStatResponse(StatResponse&& response) const {
        response_sender_.Send(std::move(response));
    }

    void RequestHandler::SendStatResponse(std::vector<StatResponse>&& responses) const {
        response_sender_.Send(std::move(responses));
    }

    bool RequestHandler::PrepareMapRendererData() {
        const data::DatabaseScheme::BusRoutesTable& buses_table = db_reader_.GetDataReader().GetBusRoutesTable();
        const data::DatabaseScheme::StopsTable& stops_table = db_reader_.GetDataReader().GetStopsTable();
        data::BusRecordSet sorted_busses;

        std::vector<data::StopRecord> stops_on_routes;
        stops_on_routes.reserve(stops_table.size());
        std::vector<geo::Coordinates> points;
        points.reserve(stops_table.size());
        std::for_each(stops_table.begin(), stops_table.end(), [&db_reader = this->db_reader_, &stops_on_routes, &points](const auto& stop) {
            if (db_reader.GetDataReader().GetBuses(&stop).empty()) {
                return;
            }
            stops_on_routes.emplace_back(&stop);
            points.emplace_back(stop.coordinates);
        });

        std::for_each(buses_table.begin(), buses_table.end(), [&sorted_busses](const auto& bus) {
            if (!bus.route.empty()) {
                sorted_busses.emplace(&bus);
            }
        });

        geo::SphereProjection projection = geo::SphereProjection::CalculateFromParams(
            std::move(points), {renderer_.GetRenderSettings().map_size}, renderer_.GetRenderSettings().padding);

        renderer_.UpdateMapProjection(std::move(projection));

        std::for_each(sorted_busses.begin(), sorted_busses.end(), [&renderer = this->renderer_](const auto& bus) {
            //! In next version need add filtering for duplicates db objects
            renderer.AddRouteToLayer(data::BusRecord{bus});
        });

        std::set<std::string>::key_compare name_comparer;
        std::sort(stops_on_routes.begin(), stops_on_routes.end(), [&name_comparer](const auto& lhs, const auto& rhs) {
            return name_comparer(lhs->name, rhs->name);
        });

        std::for_each(stops_on_routes.begin(), stops_on_routes.end(), [&renderer = this->renderer_](const auto& stop) {
            //! In next version need add filtering for duplicates db objects
            renderer.AddStopToLayer(data::StopRecord{stop});
        });

        return true;
    }

    //! Used for testing only
    std::vector<svg::Document*> RequestHandler::RenderMapByLayers(bool force_prepare_data) {
        if (force_prepare_data) {
            PrepareMapRendererData();
        }
        return std::vector<svg::Document*>{
            &renderer_.GetRouteLayer(), &renderer_.GetRouteNamesLayer(), &renderer_.GetStopMarkersLayer(), &renderer_.GetStopMarkerNamesLayer()};
    }

    std::string RequestHandler::RenderMap() {
        PrepareMapRendererData();
        return renderer_.GetRawMap();
    }
}

namespace transport_catalogue::io /* RequestHandler::SettingsBuilder implementation */ {

    maps::RenderSettings RequestHandler::SettingsBuilder::BuildMapRenderSettings(RenderSettingsRequest&& request) {
        maps::RenderSettings settings;

        assert(request.GetHeight().has_value() && request.GetWidth().has_value());
        settings.map_size = {std::move(request.GetHeight()).value(), std::move(request.GetWidth().value())};

        settings.line_width = std::move(request.GetLineWidth().value_or(0.));
        settings.padding = std::move(request.GetPadding().value_or(0.));
        settings.underlayer_width = std::move(request.GetUnderlayerWidth().value_or(0.));

        auto raw_underlayer_color = std::move(request.GetUnderlayerColor());
        if (!raw_underlayer_color.has_value()) {
            settings.underlayer_color = {};
        } else {
            std::optional<maps::Color> color = ConvertColor(std::move(raw_underlayer_color.value()));

            assert(color.has_value());
            settings.underlayer_color = std::move(color.value());
        }

        settings.stop_label_font_size = std::move(request.GetStopLabelFontSize().value_or(0));
        auto stop_label_offset = std::move(request.GetStopLabelOffset());
        if ((assert(stop_label_offset.has_value()), stop_label_offset.has_value())) {
            settings.stop_label_offset = {stop_label_offset.value()[0], stop_label_offset.value()[1]};
        }

        settings.bus_label_font_size = std::move(request.GetBusLabelFontSize().value_or(0));
        auto bus_label_offset = std::move(request.GetBusLabelOffset());
        if ((assert(bus_label_offset.has_value()), bus_label_offset.has_value())) {
            settings.bus_label_offset = {bus_label_offset.value()[0], bus_label_offset.value()[1]};
        }

        settings.stop_marker_radius = std::move(request.GetStopRadius().value_or(0.));

        auto raw_color_palette = std::move(request.GetColorPalette());
        if (raw_color_palette.has_value()) {
            std::for_each(
                std::make_move_iterator(raw_color_palette->begin()), std::make_move_iterator(raw_color_palette->end()),
                [&color_palette = settings.color_palette](auto&& raw_color) {
                    auto color = ConvertColor(std::move(raw_color));
                    color_palette.emplace_back((assert(color.has_value()), std::move(color.value())));
                });
        }

        return settings;
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

    std::optional<std::string>& StatRequest::GetName() {
        return name_;
    }

    const std::optional<std::string>& StatRequest::GetName() const {
        return name_;
    }

    void StatRequest::Build() {
        name_ = args_.ExtractIf<std::string>(StatRequestFields::NAME);
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

    bool Response::IsRouteResponse() const {
        return command_ == RequestCommand::ROUTE;
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
        std::optional<data::StopStat>&& stop_stat, std::optional<RawMapData>&& map_data, std::optional<RouteInfo>&& route_info)
        : Response(std::move(request_id), std::move(command), std::move(name)),
          bus_stat_{std::move(bus_stat)},
          stop_stat_{std::move(stop_stat)},
          map_data_{std::move(map_data)},
          route_info_{std::move(route_info)} {}

    StatResponse::StatResponse(
        StatRequest&& request, std::optional<data::BusStat>&& bus_stat, std::optional<data::StopStat>&& stop_stat,
        std::optional<RawMapData>&& map_data, std::optional<RouteInfo>&& route_info)
        : StatResponse(
              std::move((request.GetRequestId().value())), std::move(request.GetCommand()),
              request.GetName().has_value() ? std::move(request.GetName().value()) : std::string{}, std::move(bus_stat), std::move(stop_stat),
              std::move(map_data), std::move(route_info)) {}

    std::optional<data::BusStat>& StatResponse::GetBusInfo() {
        return bus_stat_;
    }

    std::optional<data::StopStat>& StatResponse::GetStopInfo() {
        return stop_stat_;
    }

    std::optional<RawMapData>& StatResponse::GetMapData() {
        return map_data_;
    }

    bool StatResponse::IsStatResponse() const {
        return true;
    }
}

namespace transport_catalogue::io /* RenderSettingsRequest implementation */ {
    bool RenderSettingsRequest::IsBaseRequest() const {
        return false;
    }

    bool RenderSettingsRequest::IsStatRequest() const {
        return false;
    }

    bool RenderSettingsRequest::IsValidRequest() const {
        throw std::runtime_error("Not implemented");
    }

    std::optional<double>& RenderSettingsRequest::GetWidth() {
        return width_;
    }
    std::optional<double>& RenderSettingsRequest::GetHeight() {
        return height_;
    }
    std::optional<double>& RenderSettingsRequest::GetPadding() {
        return padding_;
    }
    std::optional<double>& RenderSettingsRequest::GetStopRadius() {
        return stop_radius_;
    }
    std::optional<double>& RenderSettingsRequest::GetLineWidth() {
        return line_width_;
    }
    std::optional<int>& RenderSettingsRequest::GetBusLabelFontSize() {
        return bus_label_font_size_;
    }
    std::optional<RenderSettingsRequest::Offset>& RenderSettingsRequest::GetBusLabelOffset() {
        return bus_label_offset_;
    }
    std::optional<int>& RenderSettingsRequest::GetStopLabelFontSize() {
        return stop_label_font_size_;
    }
    std::optional<RenderSettingsRequest::Offset>& RenderSettingsRequest::GetStopLabelOffset() {
        return stop_label_offset_;
    }
    std::optional<double>& RenderSettingsRequest::GetUnderlayerWidth() {
        return underlayer_width_;
    }
    std::optional<RenderSettingsRequest::Color>& RenderSettingsRequest::GetUnderlayerColor() {
        return underlayer_color_;
    }
    std::optional<std::vector<RenderSettingsRequest::Color>>& RenderSettingsRequest::GetColorPalette() {
        return color_palette_;
    }

    void RenderSettingsRequest::Build() {
        width_ = args_.ExtractNumberValueIf(RenderSettingsRequestFields::WIDTH);
        height_ = args_.ExtractNumberValueIf(RenderSettingsRequestFields::HEIGHT);
        padding_ = args_.ExtractNumberValueIf(RenderSettingsRequestFields::PADDING);
        stop_radius_ = args_.ExtractNumberValueIf(RenderSettingsRequestFields::STOP_RADIUS);
        line_width_ = args_.ExtractNumberValueIf(RenderSettingsRequestFields::LINE_WIDTH);
        bus_label_font_size_ = args_.ExtractIf<int>(RenderSettingsRequestFields::BUS_LABEL_FONT_SIZE);
        bus_label_offset_ = args_.ExtractOffestValueIf(RenderSettingsRequestFields::BUS_LABEL_OFFSET);
        stop_label_font_size_ = args_.ExtractIf<int>(RenderSettingsRequestFields::STOP_LABEL_FONT_SIZE);
        stop_label_offset_ = args_.ExtractOffestValueIf(RenderSettingsRequestFields::STOP_LABEL_OFFSET);
        underlayer_color_ = args_.ExtractColorValueIf(RenderSettingsRequestFields::UNDERLAYER_COLOR);
        underlayer_width_ = args_.ExtractNumberValueIf(RenderSettingsRequestFields::UNDERLAYER_WIDTH);
        color_palette_ = args_.ExtractColorPaletteIf(RenderSettingsRequestFields::COLOR_PALETTE);
    }
}