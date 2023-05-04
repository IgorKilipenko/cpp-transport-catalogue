#pragma once

#include "request.h"

namespace transport_catalogue::io /* Response */ {
    using ResponseBase = RequestBase;
    class RawResponse : public ResponseBase {
        using ResponseBase::unordered_map;
    };

    class Response {
    public:
        using ResponseArgsMap = RawResponse;
        Response(int&& request_id, RequestCommand&& command, std::string&& name)
            : request_id_{std::move(request_id)}, command_{std::move(command)}, name_{std::move(name)} {}

        int& GetRequestId();
        int GetRequestId() const;

        virtual bool IsBusResponse() const;
        virtual bool IsStopResponse() const;
        virtual bool IsMapResponse() const;
        virtual bool IsRouteResponse() const;
        virtual bool IsStatResponse() const;
        virtual bool IsBaseResponse() const;

    protected:
        int request_id_;
        RequestCommand command_;
        std::string name_;
    };

    class StatResponse final : public Response {
    public:
        StatResponse(StatRequest&& request) : StatResponse(std::move(request), std::nullopt, std::nullopt, std::nullopt) {}

        StatResponse(
            int&& request_id, RequestCommand&& command, std::string&& name, std::optional<data::BusStat>&& bus_stat = std::nullopt,
            std::optional<data::StopStat>&& stop_stat = std::nullopt, std::optional<RawMapData>&& map_data = std::nullopt,
            std::optional<RouteInfo>&& route_info = std::nullopt);

        StatResponse(
            StatRequest&& request, std::optional<data::BusStat>&& bus_stat = std::nullopt, std::optional<data::StopStat>&& stop_stat = std::nullopt,
            std::optional<RawMapData>&& map_data = std::nullopt, std::optional<RouteInfo>&& route_info = std::nullopt);

        std::optional<data::BusStat>& GetBusInfo();
        std::optional<data::StopStat>& GetStopInfo();
        std::optional<RawMapData>& GetMapData();
        std::optional<RouteInfo>& GetRouteInfo();

        bool IsStatResponse() const override;

    private:
        std::optional<data::BusStat> bus_stat_;
        std::optional<data::StopStat> stop_stat_;
        std::optional<RawMapData> map_data_;
        std::optional<RouteInfo> route_info_;
    };
}