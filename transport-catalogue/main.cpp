#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

#include "./tests/json_test.h"
#include "./tests/svg_test.h"
#include "./tests/transport_catalogue_test.h"
#include "json_reader.h"
#include "request_handler.h"
#include "transport_catalogue.h"

/*
 * Примерная структура программы:
 *
 * Считать JSON из stdin
 * Построить на его основе JSON базу данных транспортного справочника
 * Выполнить запросы к справочнику, находящиеся в массиве "stat_requests", построив JSON-массив
 * с ответами.
 * Вывести в stdout ответы в виде JSON
 */

int main() {
    using namespace transport_catalogue::tests;
    using namespace svg::tests;
    using namespace json::tests;

    SvgTester svg_tester;
    svg_tester.TestSvglib();

    JsonTester json_tester;
    json_tester.TestJsonlib();

    TransportCatalogueTester tester;
    tester.TestTransportCatalogue();

    std::shared_ptr<io::RequestHandler> main_request_handler_ptr;
    std::string json_file = transport_catalogue::detail::io::FileReader::Read(
        std::filesystem::current_path() /= "transport-catalogue/tests/data/json_requests/test2.json");
    std::stringstream istream;
    io::JsonReader json_reader{istream};
    istream << json_file << std::endl;
    TransportCatalogue catalog;
    io::JsonResponseSender stat_sender(std::cerr);
    io::renderer::MapRenderer renderer;
    main_request_handler_ptr = std::make_shared<io::RequestHandler>(catalog.GetStatDataReader(), catalog.GetDataWriter(), stat_sender, renderer);
    json_reader.AddObserver(main_request_handler_ptr);
    json_reader.ReadDocument();
    {
        TransportCatalogue catalog;
        json_file = transport_catalogue::detail::io::FileReader::Read(
            std::filesystem::current_path() /= "transport-catalogue/tests/data/json_requests/test1.json");
        istream << json_file;
        // std::cerr << istream.str() << std::endl;
        const auto request_handler_ptr = std::make_shared<io::RequestHandler>(catalog.GetStatDataReader(), catalog.GetDataWriter(), stat_sender, renderer);
        json_reader.AddObserver(request_handler_ptr);
        json_reader.ReadDocument();
    }
    json_file = transport_catalogue::detail::io::FileReader::Read(
        std::filesystem::current_path() /= "transport-catalogue/tests/data/json_requests/test3.json");
    istream << json_file;
    json_reader.ReadDocument();

    return 0;
}
