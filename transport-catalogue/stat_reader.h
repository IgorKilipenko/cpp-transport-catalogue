#pragma once

#include <iomanip>
#include <iostream>
#include <ostream>

#include "input_reader.h"
#include "transport_catalogue.h"

namespace transport_catalogue::io {
    class StatReader {
    public:
        StatReader(TransportCatalogue::Database &catalog_db, const Reader &reader, std::ostream &out_stream = std::cout)
            : reader_{reader}, catalog_db_{catalog_db}, out_stream_{out_stream} {}

        void PrintBusInfo(const std::string_view bus_name) const;

        void PrintStopInfo(const std::string_view stop_name) const;

        void PorccessGetRequests(size_t n) const;

        void PorccessRequests() const;

        void ExecuteRequest(const Parser::RawRequest &raw_req) const;

    private:
        const Reader &reader_;
        TransportCatalogue::Database &catalog_db_;
        std::ostream &out_stream_;
    };

}
