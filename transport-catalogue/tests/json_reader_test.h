#pragma once

#include <algorithm>
#include <cassert>
#include <iostream>
#include <istream>
#include <iterator>
#include <numeric>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>

#include "../json_reader.h"
#include "../request_handler.h"

namespace transport_catalogue::tests {
    using namespace std::literals;

    class JsonReaderTester {
    private:
        class MockRequestObserver : public io::IRequestObserver {
        public:
            MockRequestObserver(std::ostream& out, std::string name) : out_(out), name_{name}, base_requests_{std::make_shared<json::Array>()} {}

            void OnBaseRequest(std::vector<io::RawRequest>&& requests) const override {
                out_ << "Has " << requests.size() << " requests" << std::endl;
                if (requests.empty()) {
                    return;
                }
                out_ << "Base requests:" << std::endl;

                json::Array array;
                std::for_each(std::make_move_iterator(requests.begin()), std::make_move_iterator(requests.end()), [&array](auto&& req) {
                    json::Dict dict;
                    std::for_each(std::make_move_iterator(req.begin()), std::make_move_iterator(req.end()), [&dict](auto&& req_val) {
                        std::string key = std::move(req_val.first);
                        json::Node::ValueType val = detail::converters::VariantCast(std::move(req_val.second));

                        std::visit(
                            [&key, &dict](auto&& val) {
                                dict.emplace(std::move(key), std::move(val));
                            },
                            std::move(val));
                    });
                    array.emplace_back(std::move(dict));
                });

                std::move(array.begin(), array.end(), std::back_inserter(*base_requests_));
            }

            void OnStatRequest(std::vector<io::RawRequest>&& /*requests*/) const override {
                out_ << "OnStatRequest" << std::endl;
            }
            void OnRenderSettingsRequest(io::RawRequest&& /*requests*/) const override {
                out_ << "OnRenderSettingsRequest" << std::endl;
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

        private:
            std::ostream& out_;
            std::string name_;
            std::shared_ptr<json::Array> base_requests_;
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
            MockRequestObserver observer(out, "Mock Observer #1");
            MockRequestObserver observer2(out, "Mock Observer #2");
            io::JsonReader json_reader{stream};

            auto subscriber = std::make_shared<MockRequestObserver>(observer);
            json_reader.AddObserver(subscriber);
            assert(json_reader.HasObserver());

            auto subscriber2 = std::make_shared<MockRequestObserver>(observer2);
            json_reader.AddObserver(subscriber2);
            assert(json_reader.HasObserver());

            json::Dict dict{
                {"base_requests",
                 json::Array{
                     json::Dict{
                         {"id", 1}, {"observer address", print_address(static_cast<const void*>(subscriber.get()))}, {"name", observer.GetName()}},
                     json::Dict{
                         {"id", 2}, {"observer address", print_address(static_cast<const void*>(subscriber2.get()))}, {"name", observer2.GetName()}},
                 }}};
            json::Document request{dict};

            request.Print(stream);

            json_reader.ReadDocument();

            {
                json::Array expected = dict["base_requests"].AsArray();
                json::Array result1 = std::move(observer.GetBaseRequests());
                json::Array result2 = std::move(observer2.GetBaseRequests());

                assert(result1 == result2 && result1 == expected);
            }

            {
                json_reader.RemoveObserver(subscriber2);
                subscriber2.reset();
                assert(json_reader.HasObserver());

                request.Print(stream);
                json_reader.ReadDocument();

                json::Array expected = dict["base_requests"].AsArray();
                json::Array result1 = std::move(observer.GetBaseRequests());
                json::Array result2 = std::move(observer2.GetBaseRequests());

                assert(result2.size() == 0);
                assert(result1.size() && result1 == expected);
            }

            {
                subscriber.reset();
                assert(!json_reader.HasObserver());

                request.Print(stream);
                json_reader.ReadDocument();

                json::Array result1 = std::move(observer.GetBaseRequests());
                json::Array result2 = std::move(observer2.GetBaseRequests());

                assert(result1.size() == 0 && result2.size() == 0);
            }
        }

        void RunTests() const {
            const std::string prefix = "[JsonReader] ";

            TestObserver();
            std::cerr << prefix << "TestObserver : Done." << std::endl;

            std::cerr << std::endl << "All JsonReader Tests : Done." << std::endl << std::endl;
        }
    };
}