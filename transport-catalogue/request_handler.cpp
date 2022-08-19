/*
 * Здесь можно было бы разместить код обработчика запросов к базе, содержащего логику, которую не
 * хотелось бы помещать ни в transport_catalogue, ни в json reader.
 *
 * Если вы затрудняетесь выбрать, что можно было бы поместить в этот файл,
 * можете оставить его пустым.
 */

#include "request_handler.h"

namespace transport_catalogue::io /* BaseRequest implementation */ {
    const std::vector<std::string>& BaseRequest::GetStops() const {
        return stops_;
    }

    std::vector<std::string>& BaseRequest::GetStops() {
        return stops_;
    }

    const std::optional<bool>& BaseRequest::IsRoundtrip() const {
        return is_roundtrip_;
    }

    std::optional<bool>& BaseRequest::IsRoundtrip() {
        return is_roundtrip_;
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

    bool BaseRequest::IsStatRequest() const {
        return false;
    }

    bool BaseRequest::IsStopCommand() const {
        return command_ == RequestCommand::STOP;
    }

    bool BaseRequest::IsBusCommand() const {
        return command_ == RequestCommand::BUS;
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
        coordinates_ = !latitude.has_value() ? std::nullopt : std::optional<Coordinates>({longitude.value(), latitude.value()});
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
            //! assert(road_distances_tmp.size() <= 2);
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

    void RequestHandler::OnBaseRequest(std::vector<RawRequest>&& requests) {
#if (TRACE && REQUEST_TRACE)
        std::cerr << "onBaseRequest" << std::endl;  //! FOR DEBUG ONLY
#endif
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

    void RequestHandler::OnBaseRequest(const std::vector<RawRequest>& requests) {
        std::vector<RawRequest> requests_tmp(requests);
        OnBaseRequest(std::move(requests_tmp));
    }

    void RequestHandler::OnStatRequest(std::vector<RawRequest>&& requests) {
#if (TRACE && REQUEST_TRACE)
        std::cerr << "onStatRequest" << std::endl;  //! FOR DEBUG ONLY
#endif
    }

    void RequestHandler::OnStatRequest([[maybe_unused]] const std::vector<RawRequest>& requests) {
        std::vector<RawRequest> requests_tmp(requests);
        OnStatRequest(std::move(requests_tmp));
    }

    void RequestHandler::ExecuteRequest(BaseRequest&& raw_req, std::vector<data::MeasuredRoadDistance>& out_distances) const {
        if (raw_req.IsStopCommand()) {
            assert(raw_req.GetCoordinates().has_value());
            db_writer_.AddStop(data::Stop{std::move(raw_req.GetName()), std::move(raw_req.GetCoordinates().value())});
            std::move(raw_req.GetroadDistances().begin(), raw_req.GetroadDistances().end(), std::back_inserter(out_distances));

        } else if (raw_req.IsBusCommand()) {
            db_writer_.AddBus(std::move(raw_req.GetName()), std::move(raw_req.GetStops()));
        }
    }

    /// Execute Basic (Insert) requests
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

        //! out_distances.shrink_to_fit();
    }

    void RequestHandler::ExecuteRequest(StatRequest&& stat_req) const {
        // StatResponse resp{stat_req.GetRequestId()};
        // stat_req.
    }

    void RequestHandler::SendStatResponse(StatResponse&& response) const {
        response_sender_.Send(std::move(response));
    }
}