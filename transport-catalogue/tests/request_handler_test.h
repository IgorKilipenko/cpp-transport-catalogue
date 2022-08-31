
#pragma once

#include <algorithm>
#include <cassert>
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
    class RequestHandlerTester {
        inline static const std::filesystem::path DATA_PATH = std::filesystem::current_path() / "transport-catalogue/tests/data/json_requests";

    public:
        void TestRequestMap() const {
            using namespace transport_catalogue::io;

            std::filesystem::path test_dir = DATA_PATH;

            std::string json_file = transport_catalogue::detail::io::FileReader::Read(test_dir / "test_map_1.json");
            std::string ecpected_json_file = transport_catalogue::detail::io::FileReader::Read(test_dir / "test_map_1__output.json");

            std::stringstream stream;
            stream << json_file;
            std::ostringstream out;

            TransportCatalogue catalog;
            JsonReader json_reader(stream);
            JsonResponseSender stat_sender(out);

            maps::MapRenderer renderer;

            const auto request_handler_ptr =
                std::make_shared<RequestHandler>(catalog.GetStatDataReader(), catalog.GetDataWriter(), stat_sender, renderer);
            json_reader.AddObserver(request_handler_ptr);

            json_reader.ReadDocument();

            std::string result = out.str();
            json::Document doc = json::Document::Load(std::stringstream{ecpected_json_file});

            std::ostringstream node_out;
            doc.Print(node_out);
            std::string expected_result = node_out.str();

            CheckResults(std::move(expected_result), std::move(result));
        }

        void RunTests() const {
            const std::string prefix = "[RequestHandler] ";

            TestRequestMap();
            std::cerr << prefix << "TestRequestMap : Done." << std::endl;

            std::cerr << std::endl << "All RequestHandler Tests : Done." << std::endl << std::endl;
        }
    };
}