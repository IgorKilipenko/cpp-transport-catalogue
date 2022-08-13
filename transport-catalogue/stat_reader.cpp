
#include "stat_reader.h"

#include <algorithm>
#include <iterator>
#include <string_view>
#include "domain.h"

namespace transport_catalogue::io {

    void StatReader::PrintBusInfo(const std::string_view bus_name) const {
        using namespace std::string_view_literals;
        const data::Bus* bus = db_reader_.GetBus(bus_name);
        out_stream_ << "Bus "sv << bus_name << ": "sv;

        if (bus == nullptr) {
            out_stream_ << "not found"sv << new_line;
        } else {
            auto info = db_stat_reader_.GetBusInfo(bus);
            out_stream_ << info.total_stops << " stops on route, "sv << info.unique_stops << " unique stops, "sv << std::setprecision(6)
                      << info.route_length << " route length, "sv << info.route_curvature << " curvature"sv << new_line;
        }
    }

    void StatReader::PrintStopInfo(const std::string_view stop_name) const {
        using namespace std::string_view_literals;

        out_stream_ << "Stop "sv << stop_name << ": "sv;

        const data::Stop* stop = db_reader_.GetStop(stop_name);
        if (stop == nullptr) {
            out_stream_ << "not found"sv << new_line;
            return;
        }

        const auto& buses = db_reader_.GetBuses(stop);
        if (buses.empty()) {
            out_stream_ << "no buses"sv << new_line;
            return;
        }

        out_stream_ << "buses"sv;
        std::for_each(buses.begin(), buses.end(), [this](const data::BusRecord& bus) {
            out_stream_ << " "sv << bus->name;
        });

        out_stream_ << std::endl;
    }

    void StatReader::PorcessRequests(size_t n) const {
        auto lines = reader_.ReadLines(n);
        const Parser& parser_ = reader_.GetParser();
        std::for_each(std::make_move_iterator(lines.begin()), std::make_move_iterator(lines.end()), [this, &parser_](const std::string_view str) {
            if (parser_.IsGetRequest(str)) {
                auto raw_req = parser_.SplitRequest(str);
                ExecuteRequest(raw_req);
            }
        });
        out_stream_.flush();
    }

    void StatReader::ExecuteRequest(const Parser::RawRequest& raw_req) const {
        assert(raw_req.type == Parser::RawRequest::Type::GET);
        assert(!raw_req.value.empty() && !raw_req.command.empty() && raw_req.args.empty());
        assert((raw_req.command == Parser::Names::BUS) || (raw_req.command == Parser::Names::STOP));

        if (raw_req.command == Parser::Names::BUS) {
            PrintBusInfo(raw_req.value);
        }
        if (raw_req.command == Parser::Names::STOP) {
            PrintStopInfo(raw_req.value);
        }
    }

    void StatReader::PorcessRequests() const {
        PorcessRequests(reader_.Read<size_t>());
    }
}