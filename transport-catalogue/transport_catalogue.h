#pragma once

#include <deque>
#include <memory>
#include <string>

#include "domain.h"

namespace transport_catalogue {
    using Coordinates = data::Coordinates;

    class TransportCatalogue : private data::ITransportDataWriter, private data::ITransportDataReader {
    public:
        using Database = data::Database<TransportCatalogue>;
        using DataTransportWriter = Database::DataWriter;
        TransportCatalogue() : db_{std::make_shared<Database>()} {}
        TransportCatalogue(std::shared_ptr<Database> db) : db_{db} {}

        void AddStop(data::Stop&& stop) const override {
            db_->AddStop(std::move(stop));
        }

        void AddStop(std::string_view name, Coordinates&& coordinates) const override {
            db_->AddStop(static_cast<std::string>(name), coordinates);
        }

        void AddBus(data::Bus&& bus) const override;

        void AddBus(std::string_view name, const std::vector<std::string_view>& stops) const override {
            db_->AddBus(static_cast<std::string>(name), stops);
        }

        const data::Bus* GetBus(const std::string_view name) const override;

        const data::Stop* GetStop(const std::string_view name) const override;

        const std::deque<data::Stop>& GetStops() const;

        const std::shared_ptr<Database> GetDatabaseForWrite() const;

        const std::shared_ptr<const Database> GetDatabaseReadOnly() const;

        const data::BusStat GetBusInfo(const data::Bus* bus) const;

        const data::ITransportDataWriter& GetWriter() const {
            return db_->GetDataWriter();
        }
        
        const data::ITransportDataReader& GetReader() const {
            return db_->GetDataReader();
        }

    private:
        std::shared_ptr<Database> db_;
    };

}

namespace transport_catalogue {}
