#pragma once

#include <deque>
#include <memory>
#include <string>

#include "domain.h"

namespace transport_catalogue {
    using Coordinates = data::Coordinates;

    class TransportCatalogue : private data::ITransportDataWriter, private data::ITransportDataReader, private data::ITransportStatDataReader {
    public:
        using Database = data::Database<TransportCatalogue>;
        using DataTransportWriter = Database::DataWriter;
        TransportCatalogue() : TransportCatalogue(std::make_shared<Database>()) {}
        TransportCatalogue(std::shared_ptr<Database> db)
            : db_{db}, db_writer_{db_->GetDataWriter()}, db_reader_{db_->GetDataReader()}, db_stat_reader_{db_reader_} {}

        void AddStop(data::Stop&& stop) const override {
            db_writer_.AddStop(std::move(stop));
        }

        void AddStop(std::string_view name, Coordinates&& coordinates) const override {
            db_writer_.AddStop(name, std::move(coordinates));
        }

        void AddBus(data::Bus&& bus) const override {
            db_->AddBus(std::move(bus));
        }

        void AddBus(std::string_view name, const std::vector<std::string_view>& stops) const override {
            db_writer_.AddBus(static_cast<std::string>(name), stops);
        }

        void SetMeasuredDistance(const std::string_view from_stop_name, const std::string_view to_stop_name, double distance) const override {
            db_writer_.SetMeasuredDistance(from_stop_name, to_stop_name, distance);
        }

        const data::Bus* GetBus(const std::string_view name) const override;

        const data::Stop* GetStop(const std::string_view name) const override;

        const std::set<std::string_view, std::less<>>& GetBuses(const data::Stop* stop) const override {
            return db_reader_.GetBuses(stop);
        }

        const std::deque<data::Stop>& GetStops() const;

        std::pair<double, double> GetDistanceBetweenStops(const data::Stop* from, const data::Stop* to) const override {
            return db_reader_.GetDistanceBetweenStops(from, to);
        }

        const std::shared_ptr<Database> GetDatabaseForWrite() const;

        const std::shared_ptr<const Database> GetDatabaseReadOnly() const;

        const data::BusStat GetBusInfo(const data::Bus* bus) const override;

        const data::ITransportDataWriter& GetDataWriter() const {
            return db_writer_;
        }

        const data::ITransportDataReader& GetDataReader() const override {
            return db_reader_;
        }

        const data::ITransportStatDataReader& GetStatDataReader() const {
            return this->db_stat_reader_;
        }

    private:
        class StatReader : public data::ITransportStatDataReader {
        public:
            StatReader(const data::ITransportDataReader& db_reader) : db_reader_{db_reader} {}
            const data::BusStat GetBusInfo(const data::Bus* bus) const override {
                data::BusStat info;
                double route_length = 0;
                double pseudo_length = 0;
                const data::Route& route = bus->route;

                for (auto i = 0; i < route.size() - 1; ++i) {
                    const data::Stop* from_stop = db_reader_.GetStop(route[i]->name);
                    const data::Stop* to_stop = db_reader_.GetStop(route[i + 1]->name);
                    assert(from_stop && from_stop);

                    const auto& [measured_dist, dist] = db_reader_.GetDistanceBetweenStops(from_stop, to_stop);
                    route_length += measured_dist;
                    pseudo_length += dist;
                }

                info.total_stops = route.size();
                info.unique_stops = std::unordered_set<const data::Stop*>(route.begin(), route.end()).size();
                info.route_length = route_length;
                info.route_curvature = route_length / std::max(pseudo_length, 1.);

                return info;
            }

            const data::ITransportDataReader& GetDataReader() const override {
                return db_reader_;
            }

        private:
            const data::ITransportDataReader& db_reader_;
        };

    private:
        std::shared_ptr<Database> db_;
        const data::ITransportDataWriter& db_writer_;
        const data::ITransportDataReader& db_reader_;
        const StatReader db_stat_reader_;
    };

}

namespace transport_catalogue {}
