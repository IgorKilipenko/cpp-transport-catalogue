#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

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
    using namespace transport_catalogue;
    using namespace transport_catalogue::io;

    TransportCatalogue catalog;
    JsonReader json_reader(std::cin);
    JsonResponseSender stat_sender(std::cout);

    maps::MapRenderer renderer;

    const auto request_handler_ptr = std::make_shared<RequestHandler>(catalog.GetStatDataReader(), catalog.GetDataWriter(), stat_sender, renderer);
    json_reader.AddObserver(request_handler_ptr);
    json_reader.ReadDocument();

        svg_map.Render(std::cout);
    */
    return 0;
}

