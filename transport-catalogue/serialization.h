#pragma once

#include <transport_catalogue.pb.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <type_traits>
#include <vector>

#include "domain.h"

namespace transport_catalogue::serialization /* Model types defs */ {
    using StopModel = proto_data_schema::Stop;
    using BusModel = proto_data_schema::Bus;
    // using DistanceBetweenStopsItem =
    //     std::pair<data::DatabaseScheme::DistanceBetweenStopsTable::key_type, data::DatabaseScheme::DistanceBetweenStopsTable::value_type>;

    struct DistanceBetweenStopsItem {
        DistanceBetweenStopsItem(data::StopRecord from_stop, data::StopRecord to_stop, double distance_between)
            : from_stop{from_stop}, to_stop{to_stop}, distance_between{distance_between} {}
        data::StopRecord from_stop;
        data::StopRecord to_stop;
        double distance_between;
    };
    //using DistanceBetweenStopsItem = data::MeasuredRoadDistance;
}

namespace transport_catalogue::serialization /* DataConvertor */ {
    class DataConvertor {
    public:
        template <typename T, std::enable_if_t<!std::is_pointer_v<std::decay_t<T>>, bool> = true>
        auto ConvertToModel(T&& object) const;

        template <typename T, std::enable_if_t<std::is_same_v<std::decay_t<T>, data::DbRecord<std::remove_pointer_t<std::decay_t<T>>>>, bool> = true>
        auto ConvertToModel(T object_ptr) const;
    };
}

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
        void SetDbPath(std::filesystem::path path) {
            db_path_ = path;
        }

        void PrepareBuses(proto_data_schema::TransportData& container) const;
        void PrepareStops(proto_data_schema::TransportData& container) const;
        void PrepareDistances(proto_data_schema::TransportData& container) const;
        proto_data_schema::TransportData BuildSerializableTransportData() const;

        bool SaveToStorage();

    public: /* deserialize methods */
        bool DeserializeTransportData() const;

    private:
        const data::ITransportStatDataReader& db_reader_;
        const data::ITransportDataWriter& db_writer_;
        const data::DatabaseScheme::StopsTable& stops_;
        std::optional<std::filesystem::path> db_path_;
        const DataConvertor convertor_;
    };

}

namespace transport_catalogue::serialization /* Store implementation */ {}