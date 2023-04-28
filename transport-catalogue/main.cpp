#include <fstream>
#include <iostream>
#include <string_view>

#include "./tests/json_reader_test.h"
#include "./tests/json_test.h"
#include "./tests/map_renderer_test.h"
#include "./tests/svg_test.h"
#include "./tests/transport_catalogue_test.h"
#include "json_reader.h"
#include "request_handler.h"
#include "tests/json_builder_test.h"
#include "tests/make_database_test.h"
#include "tests/request_handler_test.h"
#include "tests/transport_router_test.h"
#include "transport_catalogue.h"

using namespace std::literals;

int tests() {
    using namespace transport_catalogue;
    using namespace transport_catalogue::tests;
    using namespace svg::tests;
    using namespace json::tests;

    SvgTester svg_tester;
    svg_tester.TestSvglib();

    JsonTester json_tester;
    json_tester.TestJsonlib();

    JsonBuilderTester json_builder_tester;
    json_builder_tester.RunTests();

    JsonReaderTester json_reader_tester;
    json_reader_tester.RunTests();

    TransportCatalogueTester catalogue_tester;
    catalogue_tester.TestTransportCatalogue();

    MapRendererTester test_render;
    test_render.RunTests();

    RequestHandlerTester request_handler_tester;
    request_handler_tester.RunTests();

    TransportRouterTester transport_router_tester;
    transport_router_tester.RunTests();

    MakeDatabaseTester make_database_tester;
    make_database_tester.RunTests();

    return 0;
}

void Process(transport_catalogue::io::RequestHandler::Mode mode) {
    using namespace transport_catalogue;
    using namespace transport_catalogue::io;

    TransportCatalogue catalog;
    JsonReader json_reader(std::cin);
    JsonResponseSender stat_sender(std::cout);

    maps::MapRenderer renderer;

    const auto request_handler_ptr =
        std::make_shared<RequestHandler>(catalog.GetStatDataReader(), catalog.GetDataWriter(), stat_sender, renderer, mode);
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
        Process(transport_catalogue::io::RequestHandler::Mode::MAKE_BASE);

    } else if (mode == "process_requests"sv) {
        Process(transport_catalogue::io::RequestHandler::Mode::PROCESS_REQUESTS);

    } else if (mode == "tests"sv) {
        tests();

    } else {
        PrintUsage();
        return 1;
    }
}