#include <memory>
#include <sstream>

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
    std::string json_file = io::FileReader::Read("/home/igor/Documents/test1.json");
    std::stringstream istream;
    io::JsonReader json_reader{istream};
    istream << json_file << std::endl;
    TransportCatalogue catalog;
    io::renderer::MapRenderer renderer;
    main_request_handler_ptr = std::make_shared<io::RequestHandler>(catalog.GetStatDataReader(), catalog.GetDataWriter(), renderer);
    json_reader.AddObserver(main_request_handler_ptr);
    json_reader.ReadDocument();
    {
        istream << json_file;
        //std::cerr << istream.str() << std::endl;
        const auto request_handler_ptr = std::make_shared<io::RequestHandler>(catalog.GetStatDataReader(), catalog.GetDataWriter(), renderer);
        json_reader.AddObserver(request_handler_ptr);
        json_reader.ReadDocument();
    }
    istream << json_file;
    json_reader.ReadDocument();

    return 0;
}

/*
{
  "base_requests": [
    {
      "type": "Bus",
      "name": "114",
      "stops": ["Морской вокзал", "Ривьерский мост"],
      "is_roundtrip": false
    },
    {
      "type": "Stop",
      "name": "Ривьерский мост",
      "latitude": 43.587795,
      "longitude": 39.716901,
      "road_distances": {"Морской вокзал": 850}
    },
    {
      "type": "Stop",
      "name": "Морской вокзал",
      "latitude": 43.581969,
      "longitude": 39.719848,
      "road_distances": {"Ривьерский мост": 850}
    }
  ],
  "stat_requests": [
    { "id": 1, "type": "Stop", "name": "Ривьерский мост" },
    { "id": 2, "type": "Bus", "name": "114" }
  ]
}
*/