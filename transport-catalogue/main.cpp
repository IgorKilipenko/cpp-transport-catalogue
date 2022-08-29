#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

#include "./tests/json_reader_test.h"
#include "./tests/json_test.h"
#include "./tests/map_renderer_test.h"
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
    using namespace transport_catalogue;
    using namespace transport_catalogue::tests;
    using namespace svg::tests;
    using namespace json::tests;

    SvgTester svg_tester;
    svg_tester.TestSvglib();

    JsonTester json_tester;
    json_tester.TestJsonlib();

    JsonReaderTester json_reader_tester;
    json_reader_tester.RunTests();

    TransportCatalogueTester catalogue_tester;
    catalogue_tester.TestTransportCatalogue();

    MapRendererTester test_render;
    test_render.RunTests();

    return 0;
}
