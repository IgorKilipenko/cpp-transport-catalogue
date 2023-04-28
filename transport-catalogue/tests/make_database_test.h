#pragma once

#include "../json_reader.h"
#include "../request_handler.h"
#include "../transport_catalogue.h"
#include "../transport_router.h"
#include "helpers.h"

namespace transport_catalogue::tests {

    class MakeDatabaseTester {
        inline static const std::filesystem::path DATA_PATH = std::filesystem::current_path() / "transport-catalogue/tests/data/serialization";

    public:
        std::string ReadDocument(std::string json_file, io::RequestHandler::Mode mode = io::RequestHandler::Mode::MAKE_BASE) const {
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

            const auto request_handler_ptr = std::make_shared<RequestHandler>(
                catalog.GetStatDataReader(), catalog.GetDataWriter(), stat_sender, renderer, mode);
            json_reader.AddObserver(request_handler_ptr);

            json_reader.ReadDocument();
            return ostream.str();
        }

        void TestFromExample(std::string file_name, std::string answer_suffix = "expected_res") const {
            std::string result = ReadDocument(DATA_PATH / (file_name + ".json"));
            std::string expected_result = ReadDocument(DATA_PATH / (file_name + ".json"));
            // std::string expected_str = transport_catalogue::detail::io::FileReader::Read(DATA_PATH / (file_name + "_" + answer_suffix + ".json"));
            // assert(!result.empty());
        }

        void TestFromFile(std::string file_name, std::string answer_suffix = "expected_res") const {
            std::string result = ReadDocument(DATA_PATH / (file_name + ".json"), io::RequestHandler::Mode::MAKE_BASE);
            std::string request = ReadDocument(DATA_PATH / (file_name + "_request" + ".json"), io::RequestHandler::Mode::PROCESS_REQUESTS);
            std::string expected_result = transport_catalogue::detail::io::FileReader::Read(DATA_PATH / (file_name + "_" + answer_suffix + ".json"));
            assert(!result.empty());
        }

        void Test1() const {
            TestFromExample("step1_test1");
        }

        void Test2() const {
            TestFromFile("step1_test1");
        }

        /*
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
        }*/

        void RunTests() const {
            const std::string prefix = "[MakeDatabase] ";

            Test1();
            std::cerr << prefix << "Test1 : Done." << std::endl;

            Test2();
            std::cerr << prefix << "Test2 : Done." << std::endl;

            /*
            Test3();
            std::cerr << prefix << "Test3 : Done." << std::endl;

            Test4();
            std::cerr << prefix << "Test4 : Done." << std::endl;

            TestOnRandomData();
            std::cerr << prefix << "TestOnRandomData : Done." << std::endl;*/

            std::cerr << std::endl << "All MakeDatabase Tests : Done." << std::endl << std::endl;
        }
    };
}