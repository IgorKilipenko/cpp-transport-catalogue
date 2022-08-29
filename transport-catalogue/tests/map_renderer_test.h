#pragma once

#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

#include "../detail/type_traits.h"
#include "../json_reader.h"
#include "../request_handler.h"
#include "../transport_catalogue.h"
#include "helpers.h"

namespace transport_catalogue::tests {
    class MapRendererTester {
    private:
        inline static const std::string TestFile_1 =
            R"(
            {
                "width": 1200.0,
                "height": 1200.0,

                "padding": 50.0,

                "line_width": 14.0,
                "stop_radius": 5.0,

                "bus_label_font_size": 20,
                "bus_label_offset": [7.0, 15.0],

                "stop_label_font_size": 20,
                "stop_label_offset": [7.0, -3.0],

                "underlayer_color": [255, 255, 255, 0.85],
                "underlayer_width": 3.0,

                "color_palette": [
                    "green",
                    [255, 160, 0],
                    "red"
                ],
            } 
        )";

        inline static const std::string TestFile_1_Expected =
            R"~(<?xml version="1.0" encoding="UTF-8" ?>
            <svg xmlns="http://www.w3.org/2000/svg" version="1.1">
              <polyline points="99.2283,329.5 50,232.18 99.2283,329.5" fill="none" stroke="green" stroke-width="14" stroke-linecap="round" stroke-linejoin="round"/>
              <polyline points="550,190.051 279.22,50 333.61,269.08 550,190.051" fill="none" stroke="rgb(255,160,0)" stroke-width="14" stroke-linecap="round" stroke-linejoin="round"/>
            </svg>)~";

        inline static const std::string TestFile_2 = "test_1.json";

    public:
        void TestRender() const {
            using namespace transport_catalogue;
            using namespace transport_catalogue::io;

            std::filesystem::path test_dir = std::filesystem::current_path() / "transport-catalogue/tests/data/svg_render";
            std::string json_file = transport_catalogue::detail::io::FileReader::Read(test_dir / ("test_1"s + ".json"s));

            std::stringstream istream;
            std::stringstream ostream;
            istream << json_file << std::endl;

            TransportCatalogue catalog;
            JsonReader json_reader(istream);
            JsonResponseSender stat_sender(ostream);

            maps::MapRenderer renderer;

            const auto request_handler_ptr =
                std::make_shared<RequestHandler>(catalog.GetStatDataReader(), catalog.GetDataWriter(), stat_sender, renderer);
            json_reader.AddObserver(request_handler_ptr);

            json_reader.ReadDocument();
            svg::Document& map = request_handler_ptr->RenderMap();

            std::stringstream out_map_stream;
            map.Render(out_map_stream);

            std::cout << out_map_stream.str() << std::endl;
        }

        void RunTests() const {
            const std::string prefix = "[MapRenderer] ";

            TestRender();
            std::cerr << prefix << "TestRender : Done." << std::endl;

            std::cerr << std::endl << "All MapRenderer Tests : Done." << std::endl << std::endl;
        }
    };
}