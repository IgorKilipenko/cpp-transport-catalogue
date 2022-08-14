#include <cassert>
#include <iostream>

#include "../input_reader.h"
#include "../stat_reader.h"
#include "../transport_catalogue.h"

namespace transport_catalogue::tests {
    using namespace std::literals;
    using namespace transport_catalogue;

    class TransportCatalogueTester {
    public:
        void Test1() const {
            std::stringstream mainn_stream;

            std::vector<std::string> input_add_queries{
                "Stop Tolstopaltsevo: 55.611087, 37.20829, 3900m to Marushkino",
                "Stop Marushkino: 55.595884, 37.209755, 9900m to Rasskazovka, 100m to Marushkino",
                "Bus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye",
                "Bus 750: Tolstopaltsevo - Marushkino - Marushkino - Rasskazovka",
                "Stop Rasskazovka: 55.632761, 37.333324, 9500m to Marushkino",
                "Stop Biryulyovo Zapadnoye: 55.574371, 37.6517, 7500m to Rossoshanskaya ulitsa, 1800m to Biryusinka, 2400m to Universam",
                "Stop Biryusinka: 55.581065, 37.64839, 750m to Universam",
                "Stop Universam: 55.587655, 37.645687, 5600m to Rossoshanskaya ulitsa, 900m to Biryulyovo Tovarnaya",
                "Stop Biryulyovo Tovarnaya: 55.592028, 37.653656, 1300m to Biryulyovo Passazhirskaya",
                "Stop Biryulyovo Passazhirskaya: 55.580999, 37.659164, 1200m to Biryulyovo Zapadnoye",
                "Bus 828: Biryulyovo Zapadnoye > Universam > Rossoshanskaya ulitsa > Biryulyovo Zapadnoye",
                "Stop Rossoshanskaya ulitsa: 55.595579, 37.605757",
                "Stop Prazhskaya: 55.611678, 37.603831",
            };

            std::vector<std::string> input_get_queries{
                "Bus 256", "Bus 750", "Bus 751", "Stop Samara", "Stop Prazhskaya", "Stop Biryulyovo Zapadnoye ",
            };

            std::string expected_result = R"(Bus 256: 6 stops on route, 5 unique stops, 5950 route length, 1.36124 curvature)"
                                          "\n"s
                                          R"(Bus 750: 7 stops on route, 3 unique stops, 27400 route length, 1.30853 curvature)"
                                          "\n"s
                                          R"(Bus 751: not found)"
                                          "\n"s
                                          R"(Stop Samara: not found)"
                                          "\n"s
                                          R"(Stop Prazhskaya: no buses)"
                                          "\n"s
                                          R"(Stop Biryulyovo Zapadnoye: buses 256 828)"
                                          "\n"s;

            mainn_stream << input_add_queries.size() << '\n';
            std::for_each(input_add_queries.begin(), input_add_queries.end(), [&](const std::string_view line) {
                mainn_stream << line << '\n';
            });

            mainn_stream << input_get_queries.size() << '\n';
            std::for_each(input_get_queries.begin(), input_get_queries.end(), [&](const std::string_view line) {
                mainn_stream << line << '\n';
            });

            std::ostringstream output;
            TransportCatalogue catalog;
            const obsolete::io::Reader reader(catalog.GetDataWriter(), mainn_stream);
            const obsolete::io::StatReader stat_reader(catalog.GetStatDataReader(), reader, output);

            reader.PorcessRequests();
            stat_reader.PorcessRequests();

            std::string result = output.str();
            if (result != expected_result) {
                std::cerr << "Test result:" << std::endl;
                std::cerr << result << std::endl;

                std::cerr << std::endl << "Test expected result:" << std::endl;
                std::cerr << expected_result << std::endl;

                assert(false);
            }
        }

        void TestAddBus() const {
            {
                TransportCatalogue catalog;
                data::Stop stop("Stop1", {0, 0});
                data::Route route{{}};
                data::Bus bus("256"s, route);
                catalog.AddBus(data::Bus{bus});
                const data::Bus *result = catalog.GetBus(bus.name);
                assert(result && bus == *result);
            }
            // Test IDbWriter
            {
                TransportCatalogue catalog;
                const auto &db_writer = catalog.GetDataWriter();
                data::Stop stop("Stop1", {0, 0});
                data::Route route{{}};
                data::Bus bus("256"s, route);
                db_writer.AddBus(data::Bus{bus});
                const data::Bus *result = catalog.GetBus(bus.name);
                assert(result && bus == *result);
            }
        }

        void TestAddStop() const {
            TransportCatalogue catalog;
            const auto &db_writer = catalog.GetDataWriter();
            const auto &db_reader = catalog.GetDataReader();
            data::Stop expected_result{"Stop1", {94, 54}};
            db_writer.AddStop(expected_result.name, geo::Coordinates{expected_result.coordinates});
            const auto* result = db_reader.GetStop(expected_result.name);
            assert(result && expected_result == *result);
        }

        void TestTransportCatalogue() const {
            const std::string prefix = "[TransportCatalogue] ";

            Test1();
            std::cerr << prefix << "Test1 : Done." << std::endl;

            TestAddBus();
            std::cerr << prefix << "TestAddBus : Done." << std::endl;

            TestAddStop();
            std::cerr << prefix << "TestAddStop : Done." << std::endl;

            std::cerr << std::endl << "All TransportCatalogue Tests : Done." << std::endl << std::endl;
        }
    };
}