#pragma once

#include <iomanip>
#include <iostream>
#include <ostream>

#include "domain.h"
#include "input_reader.h"
#include "transport_catalogue.h"

namespace transport_catalogue::obsolete::io {
    class StatReader {
    public:
        StatReader(const data::ITransportStatDataReader &db_stat_reader, const Reader &reader, std::ostream &out_stream = std::cout)
            : reader_{reader}, db_stat_reader_{db_stat_reader}, db_reader_{db_stat_reader.GetDataReader()}, out_stream_{out_stream} {}

        void PrintBusInfo(const std::string_view bus_name) const;

        void PrintStopInfo(const std::string_view stop_name) const;

        void ProcessRequests(size_t n) const;

        void ProcessRequests() const;

        void ExecuteRequest(const Parser::RawRequest &raw_req) const;

    private:
        const Reader &reader_;
        const data::ITransportStatDataReader &db_stat_reader_;
        const data::ITransportDataReader &db_reader_;
        std::ostream &out_stream_;
        static constexpr const std::string_view new_line = "\n"sv;
    };

}
