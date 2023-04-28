#pragma once

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <functional>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "../detail/type_traits.h"
#include "../json_reader.h"
#include "../request_handler.h"
#include "../transport_catalogue.h"
#include "../transport_router.h"
#include "helpers.h"

namespace transport_catalogue::tests {

    class TransportRouterTester {
        inline static const std::filesystem::path DATA_PATH = std::filesystem::current_path() / "transport-catalogue/tests/data/router";

    public:
        std::string ReadDocument(std::string json_file) const {
            using namespace transport_catalogue;
            using namespace transport_catalogue::io;

            std::string data = transport_catalogue::detail::io::FileReader::Read(json_file);

            std::stringstream istream;
            std::stringstream ostream;
            istream << data << std::endl;

            TransportCatalogue catalog;
            JsonReader json_reader(istream);
            JsonResponseSender stat_sender(ostream);

            maps::MapRenderer renderer;

            const auto request_handler_ptr =
                std::make_shared<RequestHandler>(catalog.GetStatDataReader(), catalog.GetDataWriter(), stat_sender, renderer);
            json_reader.AddObserver(request_handler_ptr);

            json_reader.ReadDocument();
            return ostream.str();
        }

        void TestFromExample(std::string file_name, std::string answer_suffix = "output") const {
            std::string result = ReadDocument(DATA_PATH / (file_name + ".json"));
            std::string expected_str = transport_catalogue::detail::io::FileReader::Read(DATA_PATH / (file_name + "_" + answer_suffix + ".json"));
            assert(!result.empty());

            json::Document doc = json::Document::Load(std::stringstream{result});
            assert(doc.GetRoot().IsArray());
            json::Array response = doc.GetRoot().AsArray();
            json::Array expected_response = json::Node::LoadNode(std::stringstream{expected_str}).AsArray();

            assert(expected_response.size() == response.size());

            const auto calc_time = [](const json::Array& arr) -> double {
                double result = 0.;
                std::for_each(arr.begin(), arr.end(), [&result](const json::Node& node) {
                    result += node.AsMap().at("time").AsDouble();
                });
                return result;
            };

            for (auto result_it = response.begin(), expected_it = expected_response.begin(); result_it != response.end();
                 ++result_it, ++expected_it) {
                if (expected_it->IsMap() && expected_it->AsMap().count("items")) {
                    json::Dict res_map = result_it->AsMap();
                    json::Dict expected_map = expected_it->AsMap();

                    assert(res_map.count("request_id") && expected_map.count("request_id"));
                    assert(res_map.count("total_time") && expected_map.count("total_time"));

                    if (res_map != expected_map) {
                        double tt1 = calc_time(res_map.at("items").AsArray());
                        double tt2 = calc_time(expected_map.at("items").AsArray());
                        if (res_map.at("request_id") != expected_map.at("request_id") || std::abs(tt1 - tt2) > 0.011 ||
                            std::abs(res_map.at("total_time").AsDouble() - expected_map.at("total_time").AsDouble()) > 0.011) {
                            std::cerr << "Test result:" << std::endl;
                            result_it->Print(std::cerr);
                            std::cerr << std::endl;

                            std::cerr << std::endl << "Test expected result:" << std::endl;
                            expected_it->Print(std::cerr);
                            std::cerr << std::endl;
                        }
                    }
                }
            }
        }

        void Test1() const {
            TestFromExample("test1");
        }

        void Test2() const {
            TestFromExample("test2");
        }

        void Test3() const {
            TestFromExample("test3");
        }

        void Test4() const {
            TestFromExample("test4");
        }

        void TestOnRandomData() const {
            TestFromExample("s12_final_opentest_1", "answer");
            TestFromExample("s12_final_opentest_2", "answer");
            TestFromExample("s12_final_opentest_3", "answer");
        }

        void RunTests() const {
            const std::string prefix = "[TransportRouter] ";

            Test1();
            std::cerr << prefix << "Test1 : Done." << std::endl;

            Test2();
            std::cerr << prefix << "Test2 : Done." << std::endl;

            Test3();
            std::cerr << prefix << "Test3 : Done." << std::endl;

            Test4();
            std::cerr << prefix << "Test4 : Done." << std::endl;

            TestOnRandomData();
            std::cerr << prefix << "TestOnRandomData : Done." << std::endl;

            std::cerr << std::endl << "All TransportRouter Tests : Done." << std::endl << std::endl;
        }
    };
}