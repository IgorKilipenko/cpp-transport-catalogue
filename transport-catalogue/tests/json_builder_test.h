#pragma once

#include <algorithm>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "../json.h"
#include "../json_builder.h"
#include "helpers.h"

namespace transport_catalogue::tests {
    using namespace std::literals;

    class JsonBuilderTester {
    public:
        void TestDict() const {
            {
                json::Node node = json::Builder{}.StartDict().Key("key1").Value("value").EndDict().Build();
                assert(node == json::Dict({{"key1", "value"}}));
            }

            {
                json::Node node = json::Builder{}.StartDict().Key("key1").Value("value1").Key("key2").Value("value2").EndDict().Build();
                assert(node == json::Dict({{"key1", "value1"}, {"key2", "value2"}}));
            }
        }

        void TestNestedDict() const {
            {
                json::Node expected = json::Dict({{"key1", json::Dict({{"nested_key1", "nested_value1"}})}});

                json::Node node =
                    json::Builder{}.StartDict().Key("key1").StartDict().Key("nested_key1").Value("nested_value1").EndDict().EndDict().Build();

                assert(node == expected);
            }

            {
                json::Node expected = json::Dict{{"key1", json::Dict{{"nested_key1", "nested_value1"}}}, {"key2", "value2"}};

                json::Node node = json::Builder{}
                                      .StartDict()
                                      .Key("key1")
                                      .StartDict()
                                      .Key("nested_key1")
                                      .Value("nested_value1")
                                      .EndDict()
                                      .Key("key2")
                                      .Value("value2")
                                      .EndDict()
                                      .Build();

                assert(node == expected);
            }
        }

        void TestArray() const {
            {
                json::Node node = json::Builder{}.StartArray().Value("value").EndArray().Build();
                assert(node == json::Array{"value"});
            }
            {
                json::Node node = json::Builder{}
                                      .StartArray()
                                      .Value("value")
                                      .Value("value")
                                      .Value("value")
                                      .Value("value")
                                      .Value("value")
                                      .Value("value")
                                      .EndArray()
                                      .Build();
                assert(node == json::Array(6, "value"));
            }
        }

        void TestComplex() const {
            json::Node node = json::Builder{}
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
                                  .Build();
            json::Dict expected{{"key1", 123}, {"key2", "value2"}, {"key3", {json::Array{456, json::Dict{}, json::Dict{{"", nullptr}}, ""}}}};

            assert(node == expected);
        }

        void TestThrowsException() const {
            /*try {
                json::Builder{}.Build();
                assert(false);

            } catch (const std::logic_error&) {
            }*/
            /*try {
                json::Builder{}.Value(1).Value(2).Build();
                assert(false);

            } catch (const std::logic_error&) {
            }*/
            try {
                json::Builder{}.Value(json::Array(5, "v")).EndArray();
                assert(false);

            } catch (const std::logic_error&) {
            }
            try {
                json::Builder{}.Value(json::Array(5, "v")).EndDict();
                assert(false);

            } catch (const std::logic_error&) {
            }
        }

        void TestExtract() const {
            json::Builder builder;
            auto builder_context =
                builder.StartArray().Value("value").Value("value").Value("value").Value("value").Value("value").Value("value").EndArray();
            json::Node node = builder_context.Build();

            assert(node == json::Array(6, "value"));

            [[maybe_unused]] json::Builder& new_builder = static_cast<json::Builder&>(builder_context);
            assert(&new_builder == &builder_context.GetBuilder());

            json::Node expected_node = builder.Extract();
            assert(node == expected_node);

            builder.Value(json::Array(5, "v"));
            node = builder.GetContext().Build();

            assert(node == json::Array(5, "v"));
        }

        void Benchmark() const {
            std::vector<json::Builder> builders(1'000'000, json::Builder{});

            auto start = std::chrono::steady_clock::now();
            std::for_each(builders.begin(), builders.end(), [](auto& builder) {
                builder.StartDict()
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
                    .EndDict();
            });
            auto duration = std::chrono::steady_clock::now() - start;
            std::cerr << "Fill nodes time: " << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() << "ms"sv << std::endl;

            start = std::chrono::steady_clock::now();
            std::for_each(builders.begin(), builders.end(), [](auto& builder) {
                builder.GetContext().Build();
            });
            duration = std::chrono::steady_clock::now() - start;
            std::cerr << "Build nodes time: " << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() << "ms"sv << std::endl;
        }

        void RunTests() const {
            const std::string prefix = "[JsonBuilder] ";

            TestDict();
            std::cerr << prefix << "TestDict : Done." << std::endl;

            TestNestedDict();
            std::cerr << prefix << "TestNestedDict : Done." << std::endl;

            TestArray();
            std::cerr << prefix << "TestArray : Done." << std::endl;

            TestComplex();
            std::cerr << prefix << "TestComplex : Done." << std::endl;

            TestThrowsException();
            std::cerr << prefix << "TestThrowsException : Done." << std::endl;

            TestExtract();
            std::cerr << prefix << "TestExtract : Done." << std::endl;
#if (!DEBUG)
            Benchmark();
            std::cerr << prefix << "Benchmark : Done." << std::endl;
#endif

            std::cerr << std::endl << "All JsonBuilder Tests : Done." << std::endl << std::endl;
        }
    };
}