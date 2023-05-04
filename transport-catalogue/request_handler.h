#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "detail/type_traits.h"
#include "domain.h"
#include "geo.h"
#include "map_renderer.h"
#include "serialization.h"
#include "svg.h"
#include "transport_catalogue.h"
#include "transport_router.h"
#include "request.h"
#include "response.h"

namespace transport_catalogue::exceptions {
    class RequestParsingException : public std::logic_error {
    public:
        template <typename String = std::string>
        RequestParsingException(String&& message) : std::runtime_error(std::forward<String>(message)) {}
    };
}

namespace transport_catalogue::io /* Interfaces */ {
    class IRequestObserver {
    public:
        virtual void OnReadingComplete(RawRequest&& request) = 0;
        virtual void OnBaseRequest(std::vector<RawRequest>&& requests) = 0;
        virtual void OnStatRequest(std::vector<RawRequest>&& requests) = 0;
        virtual void OnRenderSettingsRequest(RawRequest&& requests) = 0;
        virtual void OnRoutingSettingsRequest(RawRequest&& requests) = 0;
        virtual void OnSerializationSettingsRequest(RawRequest&& request) = 0;

    protected:
        virtual ~IRequestObserver() = default;
    };

    class IRequestNotifier {
    public:
        virtual void AddObserver(std::shared_ptr<IRequestObserver> observer) = 0;
        virtual void RemoveObserver(std::shared_ptr<IRequestObserver> observer) = 0;
        virtual ~IRequestNotifier() = default;

    protected:
        virtual bool HasObserver() const = 0;
        virtual void NotifyReadingComplete(bool complete) = 0;
        virtual void NotifyBaseRequest(std::vector<RawRequest>&& requests) = 0;
        virtual void NotifyStatRequest(std::vector<RawRequest>&& requests) = 0;
        virtual void NotifyRenderSettingsRequest(RawRequest&& requests) = 0;
        virtual void NotifyRoutingSettingsRequest(RawRequest&& requests) = 0;
        virtual void NotifySerializationSettingsRequest(RawRequest&& requests) = 0;
    };

    class IStatResponseSender {
    public:
        virtual bool Send(StatResponse&& response) const = 0;
        virtual size_t Send(std::vector<StatResponse>&& responses) const = 0;
        virtual ~IStatResponseSender() = default;
    };
}

namespace transport_catalogue::io /* RequestHandler */ {
    class RequestHandler : public IRequestObserver {
    public: /* Constants */
        enum class Mode { MAKE_BASE, PROCESS_REQUESTS };

    public:
        RequestHandler(
            const data::ITransportStatDataReader& reader, const data::ITransportDataWriter& writer, const IStatResponseSender& response_sender,
            io::renderer::IRenderer& renderer, Mode mode = Mode::PROCESS_REQUESTS, bool force_disable_build_graph = false)
            : db_reader_{reader},
              db_writer_{writer},
              response_sender_{response_sender},
              renderer_{renderer},
              router_({}, db_reader_.GetDataReader()),
              storage_(db_reader_, db_writer_, renderer_, router_),
              mode_{mode},
              force_disable_build_graph_{force_disable_build_graph} {}

        ~RequestHandler() {}

        class RenderSettingsBuilder;

        //* Is non-const for use cache in next versions
        std::string RenderMap();

        /// Build (or if force_prepare_data = false get pre-builded) map and return non-rendered layers (as svg documents).
        ///! Used for testing only
        std::vector<svg::Document*> RenderMapByLayers(bool force_prepare_data = false);

        void OnReadingComplete(RawRequest&& request) override;
        void OnBaseRequest(std::vector<RawRequest>&& requests) override;
        void OnStatRequest(std::vector<RawRequest>&& requests) override;
        void OnRenderSettingsRequest(RawRequest&& requests) override;
        void OnRoutingSettingsRequest(RawRequest&& requests) override;
        void OnSerializationSettingsRequest(RawRequest&& request) override;

        /// Execute Basic (Insert) request
        void ExecuteRequest(BaseRequest&& raw_req, std::vector<data::MeasuredRoadDistance>& out_distances) const;

        /// Execute Basic (Insert) requests
        void ExecuteRequest(std::vector<BaseRequest>&& base_req);

        /// Execute Stat (Get) request
        void ExecuteRequest(StatRequest&& stat_req);

        /// Execute Stat (Get) requests
        void ExecuteRequest(std::vector<StatRequest>&& stat_req);

        /// Execute RenderSettings (Set) request
        void ExecuteRequest(RenderSettingsRequest&& request);

        void ExecuteRequest(RoutingSettingsRequest&& request);

        /// Execute SerializationSettings (Set) request
        void ExecuteRequest(SerializationSettingsRequest&& request);

        /// Send Stat Response
        void SendStatResponse(StatResponse&& response) const;

        /// Send Stat Response
        void SendStatResponse(std::vector<StatResponse>&& responses) const;

    private:
        const data::ITransportStatDataReader& db_reader_;
        const data::ITransportDataWriter& db_writer_;
        const IStatResponseSender& response_sender_;
        io::renderer::IRenderer& renderer_;
        router::TransportRouter router_;
        serialization::Store storage_;
        Mode mode_;
        bool force_disable_build_graph_;

        bool PrepareMapRendererData();
    };
}

namespace transport_catalogue::io /* RequestHandler::SettingsBuilder */ {

    class RequestHandler::RenderSettingsBuilder {
    public:
        static maps::RenderSettings BuildMapRenderSettings(RenderSettingsRequest&& request);

        template <typename RawColor_, detail::EnableIf<!std::is_lvalue_reference_v<RawColor_>> = true>
        static std::optional<maps::Color> ConvertColor(RawColor_&& raw_color);
    };
}

namespace transport_catalogue::io /* RequestHandler::RenderSettingsBuilder template implementation */ {

    template <typename RawColor_, detail::EnableIf<!std::is_lvalue_reference_v<RawColor_>>>
    std::optional<maps::Color> RequestHandler::RenderSettingsBuilder::ConvertColor(RawColor_&& raw_color) {
        return svg::Colors::ConvertColor(std::move(raw_color));
    }
}
