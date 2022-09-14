#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

#include "./tests/json_reader_test.h"
#include "./tests/json_test.h"
#include "./tests/map_renderer_test.h"
#include "./tests/svg_test.h"
#include "./tests/transport_catalogue_test.h"
#include "json_reader.h"
#include "request_handler.h"
#include "tests/json_builder_test.h"
#include "tests/request_handler_test.h"
#include "tests/transport_router_test.h"
#include "transport_catalogue.h"

int main() {
    using namespace transport_catalogue;
    using namespace transport_catalogue::tests;
    using namespace svg::tests;
    using namespace json::tests;

    /*SvgTester svg_tester;
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
    request_handler_tester.RunTests();*/

    TransportRouterTester transport_router_tester;
    transport_router_tester.RunTests();

    /*
        using namespace transport_catalogue;
        using namespace transport_catalogue::io;

        TransportCatalogue catalog;
        JsonReader json_reader(std::cin);
        JsonResponseSender stat_sender(std::cout);

        maps::MapRenderer renderer;

        const auto request_handler_ptr = std::make_shared<RequestHandler>(catalog.GetStatDataReader(), catalog.GetDataWriter(), stat_sender,
       renderer); json_reader.AddObserver(request_handler_ptr);

        json_reader.ReadDocument();

        std::vector<svg::Document*> layer_ptrs = request_handler_ptr->RenderMap();

        svg::Document svg_map;
        std::for_each(std::make_move_iterator(layer_ptrs.begin()), std::make_move_iterator(layer_ptrs.end()), [&svg_map](svg::Document*&& layer) {
            svg_map.MoveObjectsFrom(std::move(*layer));
        });

        svg_map.Render(std::cout);
    */
    return 0;
}