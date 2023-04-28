#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

#include "json_reader.h"
#include "request_handler.h"
#include "transport_catalogue.h"
#include "transport_catalogue.pb.h"

using namespace std::literals;

void ProcessRequests() {
    using namespace transport_catalogue;
    using namespace transport_catalogue::io;

    TransportCatalogue catalog;
    JsonReader json_reader(std::cin);
    JsonResponseSender stat_sender(std::cout);

    maps::MapRenderer renderer;

    const auto request_handler_ptr = std::make_shared<RequestHandler>(
        catalog.GetStatDataReader(), catalog.GetDataWriter(), stat_sender, renderer, RequestHandler::Mode::PROCESS_REQUESTS);
    json_reader.AddObserver(request_handler_ptr);
    json_reader.ReadDocument();
}

void MakeDataBase() {
    using namespace transport_catalogue;
    using namespace transport_catalogue::io;

    TransportCatalogue catalog;
    JsonReader json_reader(std::cin);
    JsonResponseSender stat_sender(std::cout);

    maps::MapRenderer renderer;

    const auto request_handler_ptr = std::make_shared<RequestHandler>(
        catalog.GetStatDataReader(), catalog.GetDataWriter(), stat_sender, renderer, RequestHandler::Mode::MAKE_BASE);
    json_reader.AddObserver(request_handler_ptr);
    json_reader.ReadDocument();
}

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    const std::string_view mode(argv[1]);

    if (mode == "make_base"sv) {
        // make base here
        MakeDataBase();

    } else if (mode == "process_requests"sv) {
        ProcessRequests();

    } else {
        PrintUsage();
        return 1;
    }
}