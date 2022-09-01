#pragma once

#include <cassert>
#include <iostream>
#include <vector>

#include "../json.h"
#include "../json_builder.h"
#include "helpers.h"

namespace transport_catalogue::tests {
    using namespace std::literals;

    class JsonBuilderTester {
    public:
        void TestDict() const {
            using namespace std;

            /*json::Print(
                json::Document{json::Builder{}
                                   .StartDict()
                                   .Key("key1"s)
                                   .Value(123)
                                   .Key("key2"s)
                                   .Value("value2"s)
                                   .Key("key3"s)
                                   .StartArray()
                                   .Value(456)
                                   .StartDict()
                                   .EndDict()
                                   .StartDict()
                                   .Key(""s)
                                   .Value(nullptr)
                                   .EndDict()
                                   .Value(""s)
                                   .EndArray()
                                   .EndDict()
                                   .Build()},
                cout);
            cout << endl;

            json::Print(json::Document{json::Builder{}.Value("just a string"s).Build()}, cout);
            cout << endl;*/
            {
                json::Node node = json::Builder{}.StartDict().Key("key1").Value("value").EndDict().Build();
                assert(node == json::Dict({{"key1", "value"}}));
            }

            {
                json::Node expected = json::Dict({{"key1", json::Dict({{"nested_key1", "nested_value1"}})}});

                json::Node node =
                    json::Builder{}.StartDict().Key("key1").StartDict().Key("nested_key1").Value("nested_value1").EndDict().EndDict().Build();

                assert(node == expected);
            }
        }

        void RunTests() const {
            const std::string prefix = "[JsonBuilder] ";

            TestDict();
            std::cerr << prefix << "TestDict : Done." << std::endl;

            std::cerr << std::endl << "All JsonBuilder Tests : Done." << std::endl << std::endl;
        }
    };
}