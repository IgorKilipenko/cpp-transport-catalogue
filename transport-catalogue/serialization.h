#pragma once

#include <transport_catalogue.pb.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <type_traits>
#include <vector>

#include "domain.h"

namespace transport_catalogue::serialization /* Store */ {

    class Store {
    public: /* constructors */
        Store(const data::ITransportStatDataReader& db_reader, const data::ITransportDataWriter& db_writer)
            : db_reader_{db_reader}, db_writer_{db_writer}, stops_{db_reader.GetDataReader().GetStopsTable()} {}

        Store(const Store&) = delete;
        Store(Store&& other) = delete;
        Store& operator=(const Store&) = delete;
        Store& operator=(Store&&) = delete;

    public: /* serialize methods */
        void SerializeBuses(data::BusRecord bus, std::ostream& out) const;
        void SerializeBuses(const data::Bus& bus, std::ostream& out) const;
        void SerializeBuses(std::ostream& out) const;

        void SerializeStops(data::StopRecord stop, std::ostream& out);
        void SerializeStops(const data::Stop& stop, std::ostream& out);
        void SerializeStops(std::ostream& out);

        template <typename T, std::enable_if_t<!std::is_pointer_v<std::decay_t<T>>, bool> = true>
        auto ConvertToSerializable(T&& object) const;

        template <typename T, std::enable_if_t<std::is_same_v<std::decay_t<T>, data::DbRecord<std::remove_pointer_t<std::decay_t<T>>>>, bool> = true>
        auto ConvertToSerializable(T object_ptr) const;

        void SetDbPath(std::filesystem::path path) {
            db_path_ = path;
        }

        std::vector<proto_data_schema::Bus> ConvertBusesToSerializable() const;
        void ConvertBusesToSerializable(proto_data_schema::TransportData& container) const;
        void ConvertStopsToSerializable(proto_data_schema::TransportData& container) const;
        proto_data_schema::TransportData BuildSerializableTransportData() const;

        bool SaveToStorage();

    public: /* deserialize methods */
        bool DeserializeTransportData() const;

    private:
        [[maybe_unused]] const data::ITransportStatDataReader& db_reader_;
        [[maybe_unused]] const data::ITransportDataWriter& db_writer_;
        const data::DatabaseScheme::StopsTable& stops_;
        std::optional<std::filesystem::path> db_path_;
    };

}

namespace transport_catalogue::serialization /* Store implementation */ {}