#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <istream>
#include <iterator>
#include <memory>
#include <numeric>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "../json_reader.h"
#include "../request_handler.h"
#include "helpers.h"

namespace transport_catalogue::tests {
    using namespace std::literals;

    class JsonReaderTester {
    private:
        class MockRequestObserver : public io::IRequestObserver {
        public:
            // MockRequestObserver(std::ostream& out, std::string name) : out_(out), name_{name}, base_requests_{std::make_shared<json::Array>()} {}

            MockRequestObserver(std::ostream& out, std::string name)
                : out_(out),
                  name_{name},
                  base_requests_{std::make_shared<json::Array>()},
                  stat_requests_{std::make_shared<json::Array>()},
                  render_settings_requests_{std::make_shared<std::vector<json::Dict>>()} {}

            MockRequestObserver(const MockRequestObserver&) = delete;
            MockRequestObserver& operator=(const MockRequestObserver&) = delete;
            MockRequestObserver(MockRequestObserver&& other) = default;
            MockRequestObserver& operator=(MockRequestObserver&& other) = delete;

            void OnBaseRequest(std::vector<io::RawRequest>&& requests) const override {
                out_ << "Has " << requests.size() << " requests" << std::endl;
                if (requests.empty()) {
                    return;
                }
                out_ << "Base requests:" << std::endl;

                json::Array array = io::JsonReader::ConvertToJsonArray(std::move(requests));
                std::move(array.begin(), array.end(), std::back_inserter(*base_requests_));
            }

            void OnStatRequest(std::vector<io::RawRequest>&& /*requests*/) const override {
                out_ << "OnStatRequest" << std::endl;
            }
            void OnRenderSettingsRequest(io::RawRequest&& request) const override {
                out_ << "OnRenderSettingsRequest" << std::endl;
                render_settings_requests_->emplace_back(io::JsonReader::ConvertToJsonDict(std::move(request)));
            }

            std::string& GetName() {
                return name_;
            }

            const std::string& GetName() const {
                return name_;
            }

            const json::Array& GetBaseRequests() const {
                return *base_requests_;
            }

            json::Array& GetBaseRequests() {
                return *base_requests_;
            }

            const json::Array& GetStatRequests() const {
                return *stat_requests_;
            }

            json::Array& GetStatRequests() {
                return *stat_requests_;
            }

            const std::vector<json::Dict>& GetRenderSettingsRequest() const {
                return *render_settings_requests_;
            }

            std::vector<json::Dict>& GetRenderSettingsRequest() {
                return *render_settings_requests_;
            }

        private:
            std::ostream& out_;
            std::string name_;
            std::shared_ptr<json::Array> base_requests_;
            std::shared_ptr<json::Array> stat_requests_;
            std::shared_ptr<std::vector<json::Dict>> render_settings_requests_;
        };

    public:
        void TestObserver() const {
            const auto print_address = [](const void* ptr) {
                std::ostringstream oss;
                oss << ptr;
                return oss.str();
            };

            std::stringstream stream;
            std::ostringstream out;
            io::JsonReader json_reader{stream};

            auto observer_ptr = std::make_shared<MockRequestObserver>(out, "Mock Observer #1");
            json_reader.AddObserver(observer_ptr);
            assert(json_reader.HasObserver());

            auto observer2_ptr = std::make_shared<MockRequestObserver>(out, "Mock Observer #2");
            json_reader.AddObserver(observer2_ptr);
            assert(json_reader.HasObserver());

            json::Dict dict{
                {"base_requests", json::Array{
                                      json::Dict{
                                          {"id", 1},
                                          {"observer address", print_address(static_cast<const void*>(observer_ptr.get()))},
                                          {"name", observer_ptr->GetName()}},
                                      json::Dict{
                                          {"id", 2},
                                          {"observer address", print_address(static_cast<const void*>(observer2_ptr.get()))},
                                          {"name", observer2_ptr->GetName()}},
                                  }}};
            json::Document request{dict};

            request.Print(stream);

            json_reader.ReadDocument();

            {
                json::Array expected = dict["base_requests"].AsArray();
                json::Array result1 = std::move(observer_ptr->GetBaseRequests());
                json::Array result2 = std::move(observer2_ptr->GetBaseRequests());

                assert(result1 == result2 && result1 == expected);
            }

            {
                json_reader.RemoveObserver(observer2_ptr);
                observer2_ptr.reset();
                assert(json_reader.HasObserver());

                request.Print(stream);
                json_reader.ReadDocument();

                json::Array expected = dict["base_requests"].AsArray();
                json::Array result1 = std::move(observer_ptr->GetBaseRequests());

                assert(result1.size() && result1 == expected);
            }

            {
                observer_ptr.reset();
                assert(!json_reader.HasObserver());
            }
        }

        void RenderSettingsReadTest() const {
            // std::filesystem::path test_dir = std::filesystem::current_path() / "transport-catalogue/tests/data/svg_render";
            // std::string json_file = transport_catalogue::detail::io::FileReader::Read(test_dir / ("with_render_settings(test1)"s + ".json"s));

            std::string json_file =
                R"(
                    {
                        "render_settings": {
                            "width": 9289.364936976961,
                            "height": 9228.70241453055,
                            "padding": 512.5688364782626,
                            "stop_radius": 24676.062211914188,
                            "line_width": 79550.25988723598,
                            "stop_label_font_size": 79393,
                            "stop_label_offset": [-2837.3688837120862, 55117.27307444796],
                            "underlayer_color": [82, 175, 153, 0.11467454568298674],
                            "underlayer_width": 98705.69087384189,
                            "color_palette": [
                                [172, 209, 42],
                                "chocolate",
                                "lime",
                                [77, 118, 127],
                                [83, 11, 135],
                                [253, 96, 215],
                                "yellow",
                                [149, 64, 254],
                                [170, 62, 216],
                                "violet",
                                "orchid",
                                [195, 50, 57],
                                [71, 110, 248, 0.40060153878300964],
                                [120, 64, 255, 0.20123106142965919],
                                [243, 6, 118, 0.8660767928896923],
                                [225, 160, 201],
                                [165, 177, 200, 0.2069993101592872],
                                "teal",
                                "sienna",
                                "violet",
                                "magenta",
                                [248, 147, 133, 0.3385001788747196],
                                "yellow",
                                "blue",
                                [208, 233, 151],
                                [178, 250, 251, 0.6297290355468481],
                                [71, 66, 66],
                                [19, 4, 1],
                                "gray",
                                [67, 182, 238],
                                "lime",
                                "green",
                                [1, 211, 217],
                                [0, 137, 161],
                                [8, 9, 183],
                                [149, 138, 149],
                                "aqua",
                                [34, 115, 215, 0.48087088197946026],
                                [144, 113, 217],
                                "lime",
                                [67, 71, 222, 0.7625139425979613],
                                "maroon",
                                [235, 234, 241],
                                [123, 90, 188, 0.8307634715734503],
                                [82, 110, 58]
                            ],
                            "bus_label_font_size": 21548,
                            "bus_label_offset": [-39155.514259080795, -24017.89749260967]
                        }
                    }
                )";
            /*std::string json_file =
                R"(
                    {
                        "render_settings": {
                            "color_palette": [
                                [172, 209, 42],
                                "chocolate",
                                [77, 118, 127]
                            ]
                        }
                    }
                )";*/

            std::stringstream stream;
            std::ostringstream out;

            io::JsonReader json_reader{stream};
            stream << json_file << std::endl;

            auto observer_ptr = std::make_shared<MockRequestObserver>(out, "RenderSettings Mock Observer");
            json_reader.AddObserver(observer_ptr);

            json_reader.ReadDocument();

            assert(observer_ptr->GetRenderSettingsRequest().size() == 1);
            json::Node render_settings = std::move(observer_ptr->GetRenderSettingsRequest().back());
            observer_ptr->GetRenderSettingsRequest().clear();

            json::Node expected_node = json::Node::LoadNode(std::stringstream(json_file)).AsMap()["render_settings"];
            CheckResults(std::move(expected_node), std::move(render_settings));
        }

        template <typename T1, typename... T2>
        using Union = std::variant<T1, T2...>;

        void TestJsonConverter() const {
            // base
            {
                std::string expected = "from";
                std::variant<std::nullptr_t, int, std::string, bool> from = "from";
                std::variant<std::nullptr_t, int, std::string> to = detail::converters::VariantCast(std::move(from));
                assert(std::get_if<std::string>(&to));
                assert(std::get<std::string>(to) == expected);
            }
            {
                using ToType = std::vector<std::string>;
                std::string expected = "success";
                std::variant<std::nullptr_t, std::string, ToType> from = ToType({expected});
                std::variant<std::nullptr_t, int, ToType> to = detail::converters::VariantCast(std::move(from));
                assert(std::get_if<ToType>(&to));
                assert(!std::get<ToType>(to).empty());
                assert(std::get<ToType>(to).front() == expected);
            }

            {
                using ToType = std::vector<std::variant<std::string, int>>;
                std::string expected = "success";
                std::variant<std::nullptr_t, std::string, ToType> from = ToType({expected});
                std::variant<std::nullptr_t, int, ToType> to = detail::converters::VariantCast(std::move(from));

                assert(std::get_if<ToType>(&to));
                assert(!std::get<ToType>(to).empty());

                ToType::value_type result = std::get<ToType>(to).front();
                assert(std::holds_alternative<std::string>(result));
                assert(std::get<std::string>(result) == expected);
            }

            {/*
                using Request = std::variant<std::nullptr_t, int, std::string, bool>;
                using NodeValueType = std::vector<std::variant<std::string, int>>;
                using Node = std::variant<std::nullptr_t, NodeValueType, std::vector<NodeValueType>>;
                Request request = "success";

                [[maybe_unused]]Union<Node, Request> union_result = detail::converters::VariantCast(std::move(request));
                
                assert(std::holds_alternative<std::string>(union_result));
                assert(std::get<std::string>(std::get<Request>(union_result)) == "success");

                */
            }
        }

        void RunTests() const {
            const std::string prefix = "[JsonReader] ";

            TestJsonConverter();
            std::cerr << prefix << "TestConvert : Done." << std::endl;

            TestObserver();
            std::cerr << prefix << "TestObserver : Done." << std::endl;

            RenderSettingsReadTest();
            std::cerr << prefix << "RenderSettingsReadTest : Done." << std::endl;

            std::cerr << std::endl << "All JsonReader Tests : Done." << std::endl << std::endl;
        }
    };
}