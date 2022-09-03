#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

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
        inline static const std::filesystem::path DATA_PATH = std::filesystem::current_path() / "transport-catalogue/tests/data/svg_render";

    public:
        std::vector<svg::Document> ReadDocument(std::string json_file) const {
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
            std::vector<svg::Document*> layer_ptrs = request_handler_ptr->RenderMapByLayers(true);
            std::vector<svg::Document> result;
            result.reserve(layer_ptrs.size());
            std::transform(layer_ptrs.begin(), layer_ptrs.end(), std::back_inserter(result), [](const auto* doc_ptr) {
                return *doc_ptr;
            });

            return result;
        }

        void TestRenderRoutes(std::string json_file, std::string expected_result) const {
            auto layers = ReadDocument(json_file);

            assert(!layers.empty());

            std::stringstream out_map_stream;
            layers.front().Render(out_map_stream);

            std::string map_result = out_map_stream.str();

            // std::cout << out_map_stream.str() << std::endl;
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

        void TestRenderRouteNames1() const {
            using namespace transport_catalogue;
            using namespace transport_catalogue::io;

            std::filesystem::path test_dir = std::filesystem::current_path() / "transport-catalogue/tests/data/svg_render";
            std::string json_file = transport_catalogue::detail::io::FileReader::Read(test_dir / TestFile_1);

            auto layers = ReadDocument(json_file);

            assert(!layers.empty() && layers.size() > 1);

            std::stringstream out_map_stream;
            layers[1].Render(out_map_stream);

            std::string map_result = out_map_stream.str();

            std::string expected_result =
                R"(<?xml version="1.0" encoding="UTF-8" ?>)"
                "\n"
                R"(<svg xmlns="http://www.w3.org/2000/svg" version="1.1">)"
                "\n"
                R"~(  <text fill="rgba(255,255,255,0.85)" stroke="rgba(255,255,255,0.85)" stroke-width="3" stroke-linecap="round" stroke-linejoin="round" x="99.2283" y="329.5" dx="7" dy="15" font-size="20" font-family="Verdana" font-weight="bold">114</text>)~"
                "\n"
                R"~(  <text fill="green" x="99.2283" y="329.5" dx="7" dy="15" font-size="20" font-family="Verdana" font-weight="bold">114</text>)~"
                "\n"
                R"~(  <text fill="rgba(255,255,255,0.85)" stroke="rgba(255,255,255,0.85)" stroke-width="3" stroke-linecap="round" stroke-linejoin="round" x="50" y="232.18" dx="7" dy="15" font-size="20" font-family="Verdana" font-weight="bold">114</text>)~"
                "\n"
                R"~(  <text fill="green" x="50" y="232.18" dx="7" dy="15" font-size="20" font-family="Verdana" font-weight="bold">114</text>)~"
                "\n"
                R"~(  <text fill="rgba(255,255,255,0.85)" stroke="rgba(255,255,255,0.85)" stroke-width="3" stroke-linecap="round" stroke-linejoin="round" x="550" y="190.051" dx="7" dy="15" font-size="20" font-family="Verdana" font-weight="bold">14</text>)~"
                "\n"
                R"~(  <text fill="rgb(255,160,0)" x="550" y="190.051" dx="7" dy="15" font-size="20" font-family="Verdana" font-weight="bold">14</text>)~"
                "\n"
                R"(</svg>)";

            // std::cout << out_map_stream.str() << std::endl;
            CheckResults(std::move(expected_result), std::move(map_result));
        }

        void TestRenderRouteNames2() const {
            using namespace transport_catalogue;
            using namespace transport_catalogue::io;

            std::filesystem::path test_dir = std::filesystem::current_path() / "transport-catalogue/tests/data/svg_render";
            std::string json_file = transport_catalogue::detail::io::FileReader::Read(test_dir / "test_route_names_2.json");

            auto layers = ReadDocument(json_file);

            assert(!layers.empty() && layers.size() > 1);

            std::stringstream out_map_stream;
            layers[1].Render(out_map_stream);

            std::string map_result = out_map_stream.str();

            std::string expected_result = transport_catalogue::detail::io::FileReader::Read(test_dir / "test_route_names_2__output.svg");

            CheckResults(std::move(expected_result), std::move(map_result));
        }

        void TestRenderRouteNamesFull() const {
            using namespace transport_catalogue;
            using namespace transport_catalogue::io;

            std::filesystem::path test_dir = std::filesystem::current_path() / "transport-catalogue/tests/data/svg_render";
            std::string json_file = transport_catalogue::detail::io::FileReader::Read(test_dir / "test_route_names_full.json");

            auto layers = ReadDocument(json_file);

            assert(!layers.empty() && layers.size() > 1);

            std::stringstream out_map_stream;
            layers[1].Render(out_map_stream);

            std::string map_result = out_map_stream.str();

            std::string expected_result = transport_catalogue::detail::io::FileReader::Read(test_dir / "test_route_names_full__output.svg");

            CheckResults(std::move(expected_result), std::move(map_result));

            /*{ // Check equal buy lines
                std::vector<std::string_view> expected_lines = SplitIntoWords(expected_result, '\n');
                std::vector<std::string_view> result_lines = SplitIntoWords(map_result, '\n');
                bool test_failed = false;
                size_t size = std::min(expected_lines.size(), result_lines.size());
                size_t line_index = 0;
                for (auto res_it = std::make_move_iterator(result_lines.begin()), expected_it = std::make_move_iterator(expected_lines.begin());
                     size-- > 0; ++res_it, ++expected_it) {
                       ++line_index;
                    if (*res_it != *expected_it) {
                        std::cerr << "Test failed: [line #]" << line_index << std::endl;
                        std::cerr << "Result line: ";
                        std::cerr << std::move(*res_it) << std::endl;
                        std::cerr << "Expected line: ";
                        std::cerr << std::move(*expected_it) << std::endl << std::endl;
                        test_failed = true;
                    }
                }

                assert(!test_failed);
                assert(expected_lines.size() == result_lines.size());
            }*/
        }

        void TestRenderRouteNames() const {
            TestRenderRouteNames1();
            TestRenderRouteNames2();
            TestRenderRouteNamesFull();
        }

        void TestRenderStopMarkers1() const {
            std::filesystem::path test_dir = DATA_PATH / "stop_markers";

            std::string json_file = transport_catalogue::detail::io::FileReader::Read(test_dir / "test_1.json");

            auto layers = ReadDocument(json_file);

            assert(!layers.empty() && layers.size() > 2);

            std::stringstream out_map_stream;
            layers[2].Render(out_map_stream);

            std::string map_result = out_map_stream.str();

            std::string expected_result = transport_catalogue::detail::io::FileReader::Read(test_dir / "test_1__output.svg");

            CheckResults(std::move(expected_result), std::move(map_result));
        }

        void TestRenderStopMarkersFull() const {
            std::filesystem::path test_dir = DATA_PATH / "stop_markers";

            std::string json_file = transport_catalogue::detail::io::FileReader::Read(test_dir / "test_full.json");

            auto layers = ReadDocument(json_file);

            assert(!layers.empty() && layers.size() > 2);

            std::stringstream out_map_stream;
            layers[2].Render(out_map_stream);

            std::string map_result = out_map_stream.str();

            std::string expected_result = transport_catalogue::detail::io::FileReader::Read(test_dir / "test_full__output.svg");

            // CheckResults(std::move(expected_result), std::move(map_result));

            {  // Check equal buy lines
                std::vector<std::string_view> expected_lines = SplitIntoWords(expected_result, '\n');
                std::vector<std::string_view> result_lines = SplitIntoWords(map_result, '\n');
                [[maybe_unused]]bool test_failed = false;
                size_t size = std::min(expected_lines.size(), result_lines.size());
                size_t line_index = 0;
                for (auto res_it = std::make_move_iterator(result_lines.begin()), expected_it = std::make_move_iterator(expected_lines.begin());
                     size-- > 0; ++res_it, ++expected_it) {
                    ++line_index;
                    if (*res_it != *expected_it) {
                        std::cerr << "Test failed: [line #" << line_index << "]" << std::endl;
                        std::cerr << "Result line: ";
                        std::cerr << std::move(*res_it) << std::endl;
                        std::cerr << "Expected line: ";
                        std::cerr << std::move(*expected_it) << std::endl << std::endl;
                        test_failed = true;
                    }
                }

                assert(!test_failed);
                assert(expected_lines.size() == result_lines.size());
            }

            CheckResults(std::move(expected_result), std::move(map_result));
        }

        void TestRenderStopMarkers() const {
            TestRenderStopMarkers1();
            TestRenderStopMarkersFull();
        }

        void TestRenderStopMarkerLables1() const {
            std::filesystem::path test_dir = DATA_PATH / "stop_markers";

            std::string json_file = transport_catalogue::detail::io::FileReader::Read(test_dir / "test_1.json");

            auto layers = ReadDocument(json_file);

            assert(!layers.empty() && layers.size() > 3);

            std::stringstream out_map_stream;
            layers[3].Render(out_map_stream);

            std::string map_result = out_map_stream.str();

            std::string expected_result = transport_catalogue::detail::io::FileReader::Read(test_dir / "test_1_lable__output.svg");

            CheckResults(std::move(expected_result), std::move(map_result));
        }

        void TestRenderStopMarkerLablesFull() const {
            std::filesystem::path test_dir = DATA_PATH / "stop_markers";

            std::string json_file = transport_catalogue::detail::io::FileReader::Read(test_dir / "test_full.json");

            auto layers = ReadDocument(json_file);

            assert(!layers.empty() && layers.size() > 3);

            std::stringstream out_map_stream;
            layers[3].Render(out_map_stream);

            std::string map_result = out_map_stream.str();

            std::string expected_result = transport_catalogue::detail::io::FileReader::Read(test_dir / "test_full_lable__output.svg");

            {  // Check equal buy lines
                std::vector<std::string_view> expected_lines = SplitIntoWords(expected_result, '\n');
                std::vector<std::string_view> result_lines = SplitIntoWords(map_result, '\n');
                [[maybe_unused]]bool test_failed = false;
                size_t size = std::min(expected_lines.size(), result_lines.size());
                size_t line_index = 0;
                for (auto res_it = std::make_move_iterator(result_lines.begin()), expected_it = std::make_move_iterator(expected_lines.begin());
                     size-- > 0; ++res_it, ++expected_it) {
                    ++line_index;
                    if (*res_it != *expected_it) {
                        std::cerr << "Test failed: [line #" << line_index << "]" << std::endl;
                        std::cerr << "Result line: ";
                        std::cerr << std::move(*res_it) << std::endl;
                        std::cerr << "Expected line: ";
                        std::cerr << std::move(*expected_it) << std::endl << std::endl;
                        test_failed = true;
                    }
                }

                assert(!test_failed);
                assert(expected_lines.size() == result_lines.size());
            }

            CheckResults(std::move(expected_result), std::move(map_result));
        }

        void TestRenderStopMarkerLables() const {
            TestRenderStopMarkerLables1();
            TestRenderStopMarkerLablesFull();
        }

        void TestRenderMap1() const {
            std::filesystem::path test_dir = DATA_PATH;

            std::string json_file = transport_catalogue::detail::io::FileReader::Read(test_dir / "test_1.json");

            auto layers = ReadDocument(json_file);

            assert(!layers.empty() && layers.size() > 3);

            svg::Document svg_map;
            std::for_each(std::make_move_iterator(layers.begin()), std::make_move_iterator(layers.end()), [&svg_map](svg::Document&& layer) {
                svg_map.MoveObjectsFrom(std::move(layer));
            });

            std::stringstream out_map_stream;
            svg_map.Render(out_map_stream);
            std::string map_result = out_map_stream.str();

            std::string expected_result = transport_catalogue::detail::io::FileReader::Read(test_dir / "test_1_map__output.svg");

            CheckResults(std::move(expected_result), std::move(map_result));
        }

        void TestRenderMapFull() const {
            std::filesystem::path test_dir = DATA_PATH;

            std::string json_file = transport_catalogue::detail::io::FileReader::Read(test_dir / "test_route_names_full.json");

            auto layers = ReadDocument(json_file);

            assert(!layers.empty() && layers.size() > 3);

            svg::Document svg_map;
            std::for_each(std::make_move_iterator(layers.begin()), std::make_move_iterator(layers.end()), [&svg_map](svg::Document&& layer) {
                svg_map.MoveObjectsFrom(std::move(layer));
            });

            std::stringstream out_map_stream;
            svg_map.Render(out_map_stream);
            std::string map_result = out_map_stream.str();

            std::string expected_result = transport_catalogue::detail::io::FileReader::Read(test_dir / "test_map_full__output.svg");

            {  // Check equal buy lines
                std::vector<std::string_view> expected_lines = SplitIntoWords(expected_result, '\n');
                std::vector<std::string_view> result_lines = SplitIntoWords(map_result, '\n');
                [[maybe_unused]]bool test_failed = false;
                size_t size = std::min(expected_lines.size(), result_lines.size());
                size_t line_index = 0;
                for (auto res_it = std::make_move_iterator(result_lines.begin()), expected_it = std::make_move_iterator(expected_lines.begin());
                     size-- > 0; ++res_it, ++expected_it) {
                    ++line_index;
                    if (*res_it != *expected_it) {
                        std::cerr << "Test failed: [line #" << line_index << "]" << std::endl;
                        std::cerr << "Result line: ";
                        std::cerr << std::move(*res_it) << std::endl;
                        std::cerr << "Expected line: ";
                        std::cerr << std::move(*expected_it) << std::endl << std::endl;
                        test_failed = true;
                    }
                }

                assert(!test_failed);
                assert(expected_lines.size() == result_lines.size());
            }

            CheckResults(std::move(expected_result), std::move(map_result));
        }

        void TestRenderMap() const {
            TestRenderMap1();
            TestRenderMapFull();
        }

        void RunTests() const {
            const std::string prefix = "[MapRenderer] ";

            TestRenderRoutes();
            std::cerr << prefix << "TestRenderRoutes : Done." << std::endl;

            TestRenderRouteNames();
            std::cerr << prefix << "TestRenderRouteNames : Done." << std::endl;

            TestRenderStopMarkers();
            std::cerr << prefix << "TestRenderStopMarkers : Done." << std::endl;

            TestRenderStopMarkerLables();
            std::cerr << prefix << "TestRenderStopMarkerLables : Done." << std::endl;

            TestRenderMap();
            std::cerr << prefix << "TestRenderMap : Done." << std::endl;

            std::cerr << std::endl << "All MapRenderer Tests : Done." << std::endl << std::endl;
        }
    };
}
