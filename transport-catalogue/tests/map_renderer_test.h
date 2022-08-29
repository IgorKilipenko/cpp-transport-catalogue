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
        inline static const std::string TestFile_1_Expected =
            R"~(<?xml version="1.0" encoding="UTF-8" ?>
            <svg xmlns="http://www.w3.org/2000/svg" version="1.1">
              <polyline points="99.2283,329.5 50,232.18 99.2283,329.5" fill="none" stroke="green" stroke-width="14" stroke-linecap="round" stroke-linejoin="round"/>
              <polyline points="550,190.051 279.22,50 333.61,269.08 550,190.051" fill="none" stroke="rgb(255,160,0)" stroke-width="14" stroke-linecap="round" stroke-linejoin="round"/>
            </svg>)~";

        inline static const std::string TestFile_2 = "with_render_settings(test23).json";
        inline static const std::string TestFile_1 = "test_1.json";

    public:
        void TestRenderRoutes(std::string json_file, std::string expected_result) const {
            using namespace transport_catalogue;
            using namespace transport_catalogue::io;

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

            std::string map_result = out_map_stream.str();

            //std::cout << out_map_stream.str() << std::endl;
            CheckResults(std::move(expected_result), std::move(map_result));
        }

        void TestRenderRoutes1() const {
            using namespace transport_catalogue;
            using namespace transport_catalogue::io;

            std::filesystem::path test_dir = std::filesystem::current_path() / "transport-catalogue/tests/data/svg_render";
            std::string json_file = transport_catalogue::detail::io::FileReader::Read(test_dir / TestFile_1);

            std::string expected_result =
                R"(<?xml version="1.0" encoding="UTF-8" ?>)"
                "\n"
                R"(<svg xmlns="http://www.w3.org/2000/svg" version="1.1">)"
                "\n"
                R"(  <polyline points="99.2283,329.5 50,232.18 99.2283,329.5" fill="none" stroke="green" stroke-width="14" stroke-linecap="round" stroke-linejoin="round"/>)"
                "\n"
                R"~(  <polyline points="550,190.051 279.22,50 333.61,269.08 550,190.051" fill="none" stroke="rgb(255,160,0)" stroke-width="14" stroke-linecap="round" stroke-linejoin="round"/>)~"
                "\n"
                R"(</svg>)";

            TestRenderRoutes(json_file, expected_result);
        }
        void TestRenderRoutes2() const {
            using namespace transport_catalogue;
            using namespace transport_catalogue::io;

            std::string json_file = R"(
                {
                    "base_requests": [
                        {
                            "type": "Bus",
                            "name": "14",
                            "stops": [
                            ],
                            "is_roundtrip": true
                        },
                        {
                            "type": "Bus",
                            "name": "114",
                            "stops": ["Morskoy vokzal", "Rivierskiy most"],
                            "is_roundtrip": false
                        },
                        {
                            "type": "Stop",
                            "name": "Rivierskiy most",
                            "latitude": 43.587795,
                            "longitude": 39.716901,
                            "road_distances": {
                                "Morskoy vokzal": 850
                            }
                        },
                        {
                            "type": "Stop",
                            "name": "Morskoy vokzal",
                            "latitude": 43.581969,
                            "longitude": 39.719848,
                            "road_distances": {
                                "Rivierskiy most": 850
                            }
                        },
                        {
                            "type": "Stop",
                            "name": "Elektroseti",
                            "latitude": 43.598701,
                            "longitude": 39.730623,
                            "road_distances": {
                                "Ulitsa Dokuchaeva": 3000,
                                "Ulitsa Lizy Chaikinoi": 4300
                            }
                        },
                        {
                            "type": "Stop",
                            "name": "Ulitsa Dokuchaeva",
                            "latitude": 43.585586,
                            "longitude": 39.733879,
                            "road_distances": {
                                "Ulitsa Lizy Chaikinoi": 2000,
                                "Elektroseti": 3000
                            }
                        },
                        {
                            "type": "Stop",
                            "name": "Ulitsa Lizy Chaikinoi",
                            "latitude": 43.590317,
                            "longitude": 39.746833,
                            "road_distances": {
                                "Elektroseti": 4300,
                                "Ulitsa Dokuchaeva": 2000
                            }
                        }
                    ],
                    "render_settings": {
                        "width": 600,
                        "height": 400,
                        "padding": 50,
                        "stop_radius": 5,
                        "line_width": 14,
                        "bus_label_font_size": 20,
                        "bus_label_offset": [7, 15],
                        "stop_label_font_size": 20,
                        "stop_label_offset": [7, -3],
                        "underlayer_color": [255, 255, 255, 0.85],
                        "underlayer_width": 3,
                        "color_palette": ["green", [255, 160, 0], "red"]
                    },
                    "stat_requests": []
                }

            )";

            std::string expected_result =
                R"(<?xml version="1.0" encoding="UTF-8" ?>)"
                "\n"
                R"(<svg xmlns="http://www.w3.org/2000/svg" version="1.1">)"
                "\n"
                R"(  <polyline points="201.751,350 50,50 201.751,350" fill="none" stroke="green" stroke-width="14" stroke-linecap="round" stroke-linejoin="round"/>)"
                "\n"
                R"(</svg>)";

            TestRenderRoutes(json_file, expected_result);
        }

        void TestRenderRoutes() const {
            TestRenderRoutes1();
            TestRenderRoutes2();
        }

        void RunTests() const {
            const std::string prefix = "[MapRenderer] ";

            TestRenderRoutes();
            std::cerr << prefix << "TestRenderRoutes : Done." << std::endl;

            std::cerr << std::endl << "All MapRenderer Tests : Done." << std::endl << std::endl;
        }
    };
}