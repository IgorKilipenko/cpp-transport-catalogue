#pragma once

#include <algorithm>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "domain.h"

namespace transport_catalogue /* TransportCatalogue */ {
    using Coordinates = data::Coordinates;

    class TransportCatalogue : private data::ITransportDataWriter, private data::ITransportDataReader, private data::ITransportStatDataReader {
    public:
        using Database = data::Database<TransportCatalogue>;
        using DataTransportWriter = Database::DataWriter;
        TransportCatalogue() : TransportCatalogue(std::make_shared<Database>()) {}
        TransportCatalogue(std::shared_ptr<Database> db)
            : db_{db},
              db_writer_{db_->GetDataWriter()},
              db_reader_{db_->GetDataReader()},
              db_stat_reader_{std::make_shared<const StatReader>(db_reader_)} {}

        const std::shared_ptr<Database> GetDatabaseForWrite() const;
        const std::shared_ptr<const Database> GetDatabaseReadOnly() const;
        const data::ITransportDataWriter& GetDataWriter() const;
        const data::ITransportDataReader& GetDataReader() const override;
        const data::ITransportStatDataReader& GetStatDataReader() const;

        void AddStop(data::Stop&& stop) const override;
        void AddStop(std::string&& name, Coordinates&& coordinates) const override;
        void AddBus(data::Bus&& bus) const override;
        void AddBus(std::string&& name, const std::vector<std::string_view>& stops) const override;
        void AddBus(std::string&& name, std::vector<std::string>&& stops) const override;
        void SetMeasuredDistance(const std::string_view from_stop_name, const std::string_view to_stop_name, double distance) const override;
        void SetMeasuredDistance(data::MeasuredRoadDistance&& distance) const override;

        data::BusRecord GetBus(const std::string_view name) const override;
        data::StopRecord GetStop(const std::string_view name) const override;
        const data::DatabaseScheme::StopsTable& GetStopsTable() const override;
        const data::BusRecordSet& GetBuses(data::StopRecord stop) const override;
        const data::BusRecordSet& GetBuses(const std::string_view bus_name) const override;
        data::DistanceBetweenStopsRecord GetDistanceBetweenStops(data::StopRecord from, data::StopRecord to) const override;
        
        data::BusStat GetBusInfo(data::BusRecord bus) const override;
        std::optional<data::BusStat> GetBusInfo(const std::string_view bus_name) const override;
        data::StopStat GetStopInfo(const data::StopRecord stop) const override;
        std::optional<data::StopStat> GetStopInfo(const std::string_view stop_name) const override;
        const data::BusRecordSet& GetBusesByStop(const std::string_view& stop_name) const override;

    private:
        class StatReader;

    private:
        const std::shared_ptr<Database> db_;
        const data::ITransportDataWriter& db_writer_;
        const data::ITransportDataReader& db_reader_;
        const std::shared_ptr<const StatReader> db_stat_reader_;
    };
}

namespace transport_catalogue /* TransportCatalogue::StatReader */ {
    class TransportCatalogue::StatReader : public data::ITransportStatDataReader {
    public:
        StatReader(const data::ITransportDataReader& db_reader) : db_reader_{db_reader} {}

        data::BusStat GetBusInfo(const data::BusRecord bus) const override;

        std::optional<data::BusStat> GetBusInfo(const std::string_view bus_name) const override;

        data::StopStat GetStopInfo(const data::StopRecord stop) const override;

        std::optional<data::StopStat> GetStopInfo(const std::string_view stop_name) const override;

        const data::BusRecordSet& GetBusesByStop(const std::string_view& stop_name) const override;

        const data::ITransportDataReader& GetDataReader() const override;

    private:
        const data::ITransportDataReader& db_reader_;

        template <typename Iterator>
        static size_t CulculateUniqueStops(Iterator begin, Iterator end);
    };
}

namespace transport_catalogue /* TransportCatalogue::StatReader template implementation */ {
    template <typename Iterator>
    size_t TransportCatalogue::StatReader::CulculateUniqueStops(Iterator begin, Iterator end) {
        return std::unordered_set<typename Iterator::value_type>(begin, end).size();
    }
}
