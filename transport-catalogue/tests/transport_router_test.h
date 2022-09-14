#pragma once

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <iostream>
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

        void Test1() const {
            std::string result = ReadDocument(DATA_PATH / "test1.json");
            std::cerr << result << std::endl;
        }

        void RunTests() const {
            const std::string prefix = "[TransportRouter] ";

            Test1();
            std::cerr << prefix << "Test1 : Done." << std::endl;

            std::cerr << std::endl << "All TransportRouter Tests : Done." << std::endl << std::endl;
        }
    };
}