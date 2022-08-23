#pragma once

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
            MockRequestObserver(std::ostream& out, std::string name) : out_(out), name_{name} {}

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
                json::Document result(std::move(array));
                result.Print(out_);
                out_ << std::endl;
            }

            /*void OnBaseRequest(const std::vector<io::RawRequest>& requests) const override{
                std::cerr << "OnBaseRequest" << std::endl;
            };*/
            void OnStatRequest(std::vector<io::RawRequest>&& /*requests*/) const override {
                std::cerr << "OnStatRequest" << std::endl;
            }
            void OnRenderSettingsRequest(std::vector<io::RawRequest>&& /*requests*/) const override {
                std::cerr << "OnRenderSettingsRequest" << std::endl;
            }
            

            std::string& GetName() {
                return name_;
            }

            const std::string& GetName() const {
                return name_;
            }

        private:
            std::ostream& out_;
            std::string name_;
        };

    public:
        void TestObserver() const {
            const auto print_address = [](const void* ptr) {
                std::ostringstream oss;
                oss << ptr;
                return oss.str();
            };

            std::stringstream stream;
            MockRequestObserver observer(std::cerr, "Mock Observer #1");
            MockRequestObserver observer2(std::cerr, "Mock Observer #2");
            io::JsonReader json_reader{stream};

            auto subscriber = std::make_shared<MockRequestObserver>(observer);
            json_reader.AddObserver(subscriber);

            auto subscriber2 = std::make_shared<MockRequestObserver>(observer2);
            json_reader.AddObserver(subscriber2);

            json::Dict dict{
                {"base_requests",
                 json::Array{json::Dict{
                     {"id", 1}, {"observer address", print_address(static_cast<const void*>(subscriber.get()))}, {"name", observer.GetName()}}}}};
            json::Document request{dict};

            request.Print(stream);

            json_reader.ReadDocument();
        }

        void RunTests() const {
            TestObserver();
        }
    };
}