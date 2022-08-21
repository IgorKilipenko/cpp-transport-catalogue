/*
 * Здесь можно было бы разместить код обработчика запросов к базе, содержащего логику, которую не
 * хотелось бы помещать ни в transport_catalogue, ни в json reader.
 *
 * Если вы затрудняетесь выбрать, что можно было бы поместить в этот файл,
 * можете оставить его пустым.
 */

#include "request_handler.h"

#include <algorithm>
#include <optional>
#include <vector>

#include "domain.h"

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
        auto stops_ptr = args_.find("stops");
        Array stops_tmp =
            stops_ptr == args_.end()
                ? Array{}
                : std::get<Array>((assert(std::holds_alternative<Array>(stops_ptr->second)), std::move(args_.extract(stops_ptr).mapped())));
        stops_.resize(stops_tmp.size());
        std::transform(stops_tmp.begin(), stops_tmp.end(), stops_.begin(), [&](auto&& stop_name) {
            assert(std::holds_alternative<std::string>(stop_name));
            return std::get<std::string>(stop_name);
        });
    }

    void BaseRequest::FillRoundtrip() {
        auto is_roundtrip_ptr = args_.find("is_roundtrip");
        is_roundtrip_ = is_roundtrip_ptr != args_.end() ? std::optional<bool>(
                                                              (assert(std::holds_alternative<bool>(is_roundtrip_ptr->second)),
                                                               std::get<bool>(std::move(args_.extract(is_roundtrip_ptr).mapped()))))
                                                        : std::nullopt;
    }

    void BaseRequest::FillCoordinates() {
        auto latitude_ptr = args_.find("latitude");
        std::optional<double> latitude = latitude_ptr != args_.end() ? std::optional<double>(
                                                                           (assert(std::holds_alternative<double>(latitude_ptr->second)),
                                                                            std::get<double>(std::move(args_.extract(latitude_ptr).mapped()))))
                                                                     : std::nullopt;
        auto longitude_ptr = args_.find("longitude");
        std::optional<double> longitude = longitude_ptr != args_.end() ? std::optional<double>(
                                                                             (assert(std::holds_alternative<double>(longitude_ptr->second)),
                                                                              std::get<double>(std::move(args_.extract(longitude_ptr).mapped()))))
                                                                       : std::nullopt;
        assert((latitude.has_value() && longitude.has_value()) || !(latitude.has_value() && longitude.has_value()));
        coordinates_ = !latitude.has_value() ? std::nullopt : std::optional<Coordinates>({latitude.value(), longitude.value()});
        assert(coordinates_.has_value());
    }

    void BaseRequest::FillRoadDistances() {
        auto road_distance_ptr = args_.find("road_distances");
        Dict road_distances_tmp =
            road_distance_ptr == args_.end()
                ? Dict{}
                : std::get<Dict>(
                      (assert(std::holds_alternative<Dict>(road_distance_ptr->second)), std::move(args_.extract(road_distance_ptr).mapped())));
        if (!road_distances_tmp.empty()) {
            for (auto&& item : road_distances_tmp) {
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
              (assert(type == TypeValues::BUS || type == TypeValues::STOP),
               TypeValues::BUS == std::move(type) ? RequestCommand::BUS : RequestCommand::STOP),
              std::move(name), std::move(args)) {}

    Request::Request(RawRequest&& raw_request)
        : Request(
              (assert(raw_request.count("type") && std::holds_alternative<std::string>(raw_request.at("type"))),
               std::get<std::string>(std::move(raw_request.extract("type").mapped()))),
              (assert(raw_request.count("name")), std::get<std::string>(std::move(raw_request.extract("name").mapped()))),
              ((assert(raw_request.size() > 0), std::move(raw_request)))) {}

    bool Request::IsBaseRequest() const {
        return false;
    }

    bool Request::IsStatRequest() const {
        return false;
    }

    bool Request::IsValidRequest() const {
        return (IsBusCommand() || IsStopCommand()) && !name_.empty();
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

namespace transport_catalogue::io /* RequestHandler implementation */ {
    std::optional<data::BusStat> RequestHandler::GetBusStat(const std::string_view bus_name) const {
        return db_reader_.GetBusInfo(bus_name);
    }

    const data::BusRecordSet& RequestHandler::GetBusesByStop(const std::string_view& stop_name) const {
        return db_reader_.GetDataReader().GetBuses(stop_name);
    }

    void RequestHandler::OnBaseRequest(std::vector<RawRequest>&& requests) const {
        std::vector<BaseRequest> reqs;
        reqs.reserve(requests.size());

        std::for_each(std::make_move_iterator(requests.begin()), std::make_move_iterator(requests.end()), [&reqs](RawRequest&& raw_req) {
            BaseRequest base_req(std::move(raw_req));
            reqs.emplace_back(std::move(base_req));
        });

        std::sort(reqs.begin(), reqs.end(), [](const BaseRequest& lhs, const BaseRequest& rhs) {
            assert(lhs.GetCommand() != RequestCommand::UNDEFINED && rhs.GetCommand() != RequestCommand::UNDEFINED);
            return static_cast<uint8_t>(lhs.GetCommand()) < static_cast<uint8_t>(rhs.GetCommand());
        });

        ExecuteRequest(std::move(reqs));
    }

    void RequestHandler::OnBaseRequest(const std::vector<RawRequest>& requests) const {
        std::vector<RawRequest> requests_tmp(requests);
        OnBaseRequest(std::move(requests_tmp));
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

    void RequestHandler::OnStatRequest([[maybe_unused]] const std::vector<RawRequest>& requests) const {
        std::vector<RawRequest> requests_tmp(requests);
        OnStatRequest(std::move(requests_tmp));
    }

    void RequestHandler::ExecuteRequest(BaseRequest&& raw_req, std::vector<data::MeasuredRoadDistance>& out_distances) const {
        assert(raw_req.IsValidRequest());

        if (raw_req.IsStopCommand()) {
            db_writer_.AddStop(data::Stop{std::move(raw_req.GetName()), std::move(raw_req.GetCoordinates().value())});
            std::move(raw_req.GetroadDistances().begin(), raw_req.GetroadDistances().end(), std::back_inserter(out_distances));

        } else {
            raw_req.ConvertToRoundtrip();
            db_writer_.AddBus(std::move(raw_req.GetName()), std::move(raw_req.GetStops()));
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
        auto request_id__ptr = args_.find("id");
        request_id_ = request_id__ptr != args_.end() ? std::optional<int>(
                                                           (assert(std::holds_alternative<int>(request_id__ptr->second)),
                                                            std::get<int>(std::move(args_.extract(request_id__ptr).mapped()))))
                                                     : std::nullopt;
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