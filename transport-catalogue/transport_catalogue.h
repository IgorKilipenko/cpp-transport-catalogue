#pragma once

#include <algorithm>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <vector>

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

        void AddStop(std::string&& name, Coordinates&& coordinates) const override {
            db_writer_.AddStop(std::move(name), std::move(coordinates));
        }

        void AddBus(data::Bus&& bus) const override {
            db_->AddBus(std::move(bus));
        }

        void AddBus(std::string&& name, const std::vector<std::string_view>& stops) const override {
            db_writer_.AddBus(std::move(name), stops);
        }

        void AddBus(std::string&& name, std::vector<std::string>&& stops) const override {
            db_writer_.AddBus(std::move(name), std::move(stops));
        }

        void SetMeasuredDistance(const std::string_view from_stop_name, const std::string_view to_stop_name, double distance) const override {
            db_writer_.SetMeasuredDistance(from_stop_name, to_stop_name, distance);
        }

        void SetMeasuredDistance(data::MeasuredRoadDistance&& distance) const override {
            db_writer_.SetMeasuredDistance(std::move(distance));
        }

        data::BusRecord GetBus(const std::string_view name) const override;

        data::StopRecord GetStop(const std::string_view name) const override;

        const data::DatabaseScheme::StopsTable& GetStopsTable() const override {
            return db_reader_.GetStopsTable();
        }

        const data::BusRecordSet& GetBuses(data::StopRecord stop) const override {
            return db_reader_.GetBuses(stop);
        }

        const data::BusRecordSet& GetBuses(const std::string_view bus_name) const override {
            return db_reader_.GetBuses(bus_name);
        }

        data::DistanceBetweenStopsRecord GetDistanceBetweenStops(data::StopRecord from, data::StopRecord to) const override {
            return db_reader_.GetDistanceBetweenStops(from, to);
        }

        const std::shared_ptr<Database> GetDatabaseForWrite() const;

        const std::shared_ptr<const Database> GetDatabaseReadOnly() const;

        data::BusStat GetBusInfo(data::BusRecord bus) const override {
            return db_stat_reader_.GetBusInfo(bus);
        }

        std::optional<data::BusStat> GetBusInfo(const std::string_view bus_name) const override {
            return db_stat_reader_.GetBusInfo(bus_name);
        }

        data::StopStat GetStopInfo(const data::StopRecord stop) const override {
            return db_stat_reader_.GetStopInfo(stop);
        }

        std::optional<data::StopStat> GetStopInfo(const std::string_view stop_name) const override {
            return db_stat_reader_.GetStopInfo(stop_name);
        }

        const data::BusRecordSet& GetBusesByStop(const std::string_view& stop_name) const override {
            return db_stat_reader_.GetBusesByStop(stop_name);
        }

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

            data::BusStat GetBusInfo(const data::BusRecord bus) const override {
                data::BusStat info;
                double route_length = 0;
                double pseudo_length = 0;
                const data::Route& route = bus->route;

                for (auto i = 0; i < route.size() - 1; ++i) {
                    data::StopRecord from_stop = db_reader_.GetStop(route[i]->name);
                    data::StopRecord to_stop = db_reader_.GetStop(route[i + 1]->name);
                    assert(from_stop && from_stop);

                    const auto& dist_btw = db_reader_.GetDistanceBetweenStops(from_stop, to_stop);
                    route_length += dist_btw.measured_distance;
                    pseudo_length += dist_btw.distance;
                }

                info.total_stops = route.size();
                info.unique_stops =
                    CulculateUniqueStops(route.begin(), route.end());  // std::unordered_set<data::StopRecord>(route.begin(), route.end()).size();
                info.route_length = route_length;
                info.route_curvature = route_length / std::max(pseudo_length, 1.);

                return info;
            }

            std::optional<data::BusStat> GetBusInfo(const std::string_view bus_name) const override {
                data::BusRecord bus = db_reader_.GetBus(bus_name);
                return bus != nullptr ? std::optional{GetBusInfo(bus)} : std::nullopt;
            }

            data::StopStat GetStopInfo(const data::StopRecord stop) const override {
                const data::BusRecordSet& buses = db_reader_.GetBuses(stop);
                std::vector<std::string> buses_names(buses.size());
                std::transform(buses.begin(), buses.end(), buses_names.begin(), [](const auto& bus) {
                    return bus->name;
                });
                return data::StopStat{std::move(buses_names)};
            }

            std::optional<data::StopStat> GetStopInfo(const std::string_view stop_name) const override {
                const data::StopRecord stop = db_reader_.GetStop(stop_name);
                if (stop == nullptr) {
                    return std::nullopt;
                }
                return std::optional<data::StopStat>{GetStopInfo(stop)};
            }

            const data::BusRecordSet& GetBusesByStop(const std::string_view& stop_name) const override {
                return db_reader_.GetBuses(stop_name);
            }

            const data::ITransportDataReader& GetDataReader() const override {
                return db_reader_;
            }

        private:
            const data::ITransportDataReader& db_reader_;

            template <typename Iterator>
            static size_t CulculateUniqueStops(Iterator begin, Iterator end) {
                return std::unordered_set<typename Iterator::value_type>(begin, end).size();
            }
        };

    private:
        std::shared_ptr<Database> db_;
        const data::ITransportDataWriter& db_writer_;
        const data::ITransportDataReader& db_reader_;
        const StatReader db_stat_reader_;
    };

}

namespace transport_catalogue {}
