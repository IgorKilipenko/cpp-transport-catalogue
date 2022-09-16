#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <deque>
#include <execution>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "detail/type_traits.h"
#include "geo.h"

namespace transport_catalogue::exceptions {
    namespace data {
        class DatabaseException : public std::exception {
        public:
            DatabaseException(std::string message) : std::exception(), message_{std::move(message)} {}
            const char* what() const noexcept {
                return message_.c_str();
            }

        private:
            std::string message_;
        };
    }
    class NotImplementedException : public std::logic_error {
    public:
        NotImplementedException() : std::logic_error("Function not yet implemented.") {}
    };
}

namespace transport_catalogue::detail::converters {

    template <class... Args>
    struct VariantCastProxy {
        std::variant<Args...> value;

        template <class... ToArgs>
        operator std::variant<ToArgs...>() const {
            return std::visit(
                [](auto&& arg) -> std::variant<ToArgs...> {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (detail::IsConvertibleV<T, std::variant<ToArgs...>>) {
                        return std::move(arg);
                    } else {
                        if constexpr (detail::IsConvertibleV<std::monostate, std::variant<ToArgs...>>) {
                            return std::monostate();
                        } else {
                            return nullptr;
                        }
                    }
                },
                value);
        }
    };

    template <class... Args>
    auto VariantCast(std::variant<Args...>&& value) -> VariantCastProxy<Args...> {
        return {std::move(value)};
    }

    template <typename ReturnType, typename Filter = ReturnType, typename... Args>
    ReturnType VariantCast(std::variant<Args...>&& value) {
        return std::visit(
            [](auto&& arg) -> ReturnType {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (detail::IsConvertibleV<T, Filter>) {
                    return std::move(arg);
                } else {
                    if constexpr (detail::IsConvertibleV<std::monostate, ReturnType>) {
                        return std::monostate();
                    } else {
                        return nullptr;
                    }
                }
            },
            value);
    }

    template <typename ExecutionPolicy, EnableForExecutionPolicy<ExecutionPolicy> = true>
    static void MakeUnique(ExecutionPolicy&& policy, std::vector<std::string_view>& values) {
        std::sort(policy, values.begin(), values.end());
        auto last = std::unique(policy, values.begin(), values.end());
        values.erase(last, values.end());
    }

    [[maybe_unused]] static void MakeUnique(std::vector<std::string_view>& values) {
        MakeUnique(std::execution::seq, values);
    }
}

namespace transport_catalogue::data /* Db objects (ORM) */ {
    using Coordinates = geo::Coordinates;

    struct Stop {
        std::string name;
        Coordinates coordinates;
        Stop() = default;
        template <
            typename String = std::string, typename Coordinates = data::Coordinates,
            detail::EnableIf<detail::IsConvertibleV<String, std::string_view> && detail::IsSameV<Coordinates, data::Coordinates>> = true>
        Stop(String&& name, Coordinates&& coordinates) : name{std::forward<String>(name)}, coordinates{std::forward<Coordinates>(coordinates)} {}

        bool operator==(const Stop& rhs) const noexcept;

        bool operator!=(const Stop& rhs) const noexcept;
    };

    template <typename T>
    using DbRecord = const T*;
    template <typename T>
    constexpr const T* DbNull = nullptr;

    using StopRecord = DbRecord<Stop>;
    using StopRecordSet = std::deque<StopRecord>;

    class Route : public std::vector<StopRecord> {
    public:
        using vector::vector;
    };

    struct Bus {
        std::string name;
        Route route;
        bool is_roundtrip = false;
        Bus() = default;
        template <
            typename String = std::string, typename Route = data::Route,
            detail::EnableIf<detail::IsConvertibleV<String, std::string> && detail::IsSameV<Route, data::Route>> = true>
        Bus(String&& name, Route&& route, bool is_roundtrip)
            : name{std::forward<String>(name)}, route{std::forward<Route>(route)}, is_roundtrip{is_roundtrip} {}

        bool operator==(const Bus& rhs) const noexcept;

        bool operator!=(const Bus& rhs) const noexcept;

        /// If IsRoundtrip is true return first Stop of route. Else return center point (for non-roundtrip route)
        /// Calling on an empty route is undefined
        const Route::value_type& GetLastStopOfRoute() const {
            assert(!route.empty());

            if (route.size() > 1 && !is_roundtrip) {
                auto center = static_cast<size_t>(route.size() / 2ul);
                return route[center];
            }
            return route.front();
        }
    };

    struct ByNameCompare {
    public:
        bool operator()(const Bus* lhs, const Bus* rhs) const {
            return string_compare_(lhs->name, rhs->name);
        }

    private:
        std::set<std::string>::key_compare string_compare_;
    };

    using BusRecord = DbRecord<Bus>;
    using BusRecordSet = std::set<BusRecord, ByNameCompare>;

    struct DistanceBetweenStopsRecord {
        double distance = 0.;
        double measured_distance = 0.;
    };

    struct MeasuredRoadDistance {
        std::string from_stop;
        std::string to_stop;
        double distance = 0.;

        template <typename String = std::string, detail::EnableIfSame<String, std::string> = true>
        MeasuredRoadDistance(String&& from_stop, String&& to_stop, double distance)
            : from_stop(std::move(from_stop)), to_stop(std::move(to_stop)), distance(distance) {}
    };

    struct BusStat {
        size_t total_stops{};
        size_t unique_stops{};
        double route_length{};
        double route_curvature{};
    };

    struct StopStat {
        std::vector<std::string> buses;
    };

    class Hasher {
    public:
        size_t operator()(const std::pair<const Stop*, const Stop*>& stops) const;

        template <typename T>
        size_t operator()(std::initializer_list<const T*> items) const {
            size_t hash = 0;
            for (const T* cur : items) {
                hash = hash * INDEX + pointer_hasher_(cur);
            }
            return hash;
        }

    private:
        std::hash<const void*> pointer_hasher_;
        static const size_t INDEX = 42;
    };

}

namespace transport_catalogue::data /* Db scheme abstraction */ {
    class DataTable {
    public:
        virtual const std::string_view GetName() const {
            return name_;
        }

    protected:
        DataTable(std::string name) : name_(std::move(name)) {}

    private:
        std::string name_;
    };

    class TableView {
    public:
        virtual const std::string_view GetName() const {
            return name_;
        }

    protected:
        TableView(const std::string name) : name_(std::move(name)) {}

    private:
        std::string name_;
    };
}

namespace transport_catalogue::data /* Db scheme */ {
    class DatabaseScheme {
    public: /* Aliases */
        using DistanceBetweenStopsTableBase = std::unordered_map<std::pair<const Stop*, const Stop*>, DistanceBetweenStopsRecord, Hasher>;
        using NameToStopViewBase = std::unordered_map<std::string_view, const data::Stop*>;
        using NameToBusRoutesViewBase = std::unordered_map<std::string_view, const data::Bus*>;
        using StopToBusesViewBase = std::unordered_map<StopRecord, BusRecordSet>;

    public:
        class StopsTable : public DataTable, public std::deque<Stop> {
        public:
            using std::deque<Stop>::deque;
            StopsTable() : DataTable("StopsTable"), std::deque<Stop>() {}
        };

        class BusRoutesTable : public DataTable, public std::deque<Bus> {
        public:
            using std::deque<Bus>::deque;
            BusRoutesTable() : DataTable("BusRoutesTable"), std::deque<Bus>() {}
        };

        class DistanceBetweenStopsTable : public DataTable, public DistanceBetweenStopsTableBase {
        public:
            using DistanceBetweenStopsTableBase::unordered_map;
            DistanceBetweenStopsTable() : DataTable("DistanceBetweenStopsTable"), DistanceBetweenStopsTableBase() {}
        };

        class NameToStopView : public TableView, public NameToStopViewBase {
        public:
            using NameToStopViewBase::unordered_map;
            NameToStopView() : TableView("NameToStopView"), NameToStopViewBase() {}
        };

        class NameToBusRoutesView : public TableView, public NameToBusRoutesViewBase {
        public:
            using NameToBusRoutesViewBase::unordered_map;
            NameToBusRoutesView() : TableView("NameToBusRoutesView"), NameToBusRoutesViewBase() {}
        };

        class StopToBusesView : public TableView, public StopToBusesViewBase {
        public:
            using StopToBusesViewBase::unordered_map;
            StopToBusesView() : TableView("StopToBusesView"), StopToBusesViewBase() {}
        };
    };
}

namespace transport_catalogue::data /* Interfaces */ {
    class ITransportDataReader {
    public:
        virtual BusRecord GetBus(std::string_view name) const = 0;
        virtual StopRecord GetStop(std::string_view name) const = 0;
        virtual const DatabaseScheme::StopsTable& GetStopsTable() const = 0;

        virtual const BusRecordSet& GetBuses(StopRecord stop) const = 0;
        virtual const BusRecordSet& GetBuses(const std::string_view bus_name) const = 0;
        virtual const DatabaseScheme::BusRoutesTable& GetBusRoutesTable() const = 0;

        virtual DistanceBetweenStopsRecord GetDistanceBetweenStops(StopRecord from, StopRecord to) const = 0;

        virtual ~ITransportDataReader() = default;
    };

    class ITransportDataWriter {
    public:
        virtual void AddBus(Bus&& bus) const = 0;
        virtual void AddBus(std::string&& name, const std::vector<std::string_view>& stops, bool is_roundtrip) const = 0;
        virtual void AddBus(std::string&& name, std::vector<std::string>&& stops, bool is_roundtrip) const = 0;

        virtual void AddStop(Stop&& stop) const = 0;
        virtual void AddStop(std::string&&, Coordinates&& coordinates) const = 0;

        virtual void SetMeasuredDistance(const std::string_view from_stop_name, const std::string_view to_stop_name, double distance) const = 0;
        virtual void SetMeasuredDistance(data::MeasuredRoadDistance&& distance) const = 0;

        virtual ~ITransportDataWriter() = default;
    };

    class ITransportStatDataReader {
    public:
        virtual BusStat GetBusInfo(const data::BusRecord bus) const = 0;
        virtual std::optional<BusStat> GetBusInfo(const std::string_view bus_name) const = 0;

        virtual StopStat GetStopInfo(const data::StopRecord stop) const = 0;
        virtual std::optional<StopStat> GetStopInfo(const std::string_view stop_name) const = 0;

        virtual const data::ITransportDataReader& GetDataReader() const = 0;

        virtual ~ITransportStatDataReader() = default;
    };
}

namespace transport_catalogue::data /* Database */ {

    template <class Owner>
    class Database {
        friend Owner;

    public:
        Database() : db_writer_{*this}, db_reader_{*this} {}

        const DatabaseScheme::StopsTable& GetStopsTable() const;

        const DatabaseScheme::BusRoutesTable& GetBusRoutesTable() const;

        const DatabaseScheme::NameToStopView& GetNameToStopView() const;

        const ITransportDataWriter& GetDataWriter() const;

        const ITransportDataReader& GetDataReader() const;

    protected: /* ORM */
        template <typename Stop = data::Stop, detail::EnableIfSame<Stop, data::Stop> = true>
        const Stop& AddStop(Stop&& stop);

        template <
            typename String = std::string, typename Coordinates = data::Coordinates,
            detail::EnableIf<detail::IsSameV<String, std::string> && detail::IsConvertibleV<Coordinates, data::Coordinates>> = true>
        const Stop& AddStop(String&& name, Coordinates&& coordinates);

        void AddMeasuredDistance(const std::string_view from_stop_name, const std::string_view to_stop_name, double distance);

        template <typename Bus, detail::EnableIfSame<Bus, data::Bus> = true>
        const Bus& AddBus(Bus&& bus);

        template <
            typename String = std::string, typename Route = data::Route,
            detail::EnableIf<detail::IsSameV<String, std::string> && detail::IsSameV<Route, data::Route>> = true>
        const Bus& AddBus(String&& name, Route&& route, bool is_roundtrip);

        template <
            typename String = std::string, typename StopsNameContainer = std::vector<std::string_view>,
            detail::EnableIf<
                detail::IsConvertibleV<String, std::string> && (detail::IsSameV<StopsNameContainer, std::vector<std::string>> ||
                                                                detail::IsSameV<StopsNameContainer, std::vector<std::string_view>>)> = true>
        const Bus& AddBus(String&& name, StopsNameContainer&& route, bool is_roundtrip);

        template <
            typename String = std::string, typename StopsNameContainer = std::vector<std::string_view>,
            detail::EnableIf<detail::IsConvertibleV<String, std::string> && detail::IsSameV<StopsNameContainer, std::vector<std::string_view>>> =
                true>
        const Bus* AddBusForce(String&& name, StopsNameContainer&& stops, bool is_roundtrip);

        const Bus* GetBus(const std::string_view name) const;

        const Stop* GetStop(const std::string_view name) const;

    private:
        DatabaseScheme::StopsTable stops_;
        DatabaseScheme::BusRoutesTable bus_routes_;
        DatabaseScheme::DistanceBetweenStopsTable measured_distances_btw_stops_;

        DatabaseScheme::NameToStopView name_to_stop_;
        DatabaseScheme::NameToBusRoutesView name_to_bus_;
        DatabaseScheme::StopToBusesView stop_to_buses_;

        std::mutex mutex_;

        template <
            typename StringView, typename TableView,
            detail::EnableIf<detail::IsConvertibleV<StringView, std::string_view> && detail::IsBaseOfV<TableView, TableView>> = true>
        const auto* GetItem(StringView&& name, const TableView& table) const;

        void LockDatabase();

        void UnlockDatabase();

        std::lock_guard<std::mutex> LockGuard();

        template <
            typename Container,
            detail::EnableIf<detail::IsSameV<Container, std::vector<std::string_view>> || detail::IsSameV<Container, std::vector<std::string>>> =
                true>
        Route ToRoute(Container&& stops) const;

    public:
        class DataWriter;
        class DataReader;

    private:
        DataWriter db_writer_;
        DataReader db_reader_;
    };
}

namespace transport_catalogue::data /* Database inner classes (Read/Write interface) */ {
    template <class Owner>
    class Database<Owner>::DataWriter : public ITransportDataWriter {
    public:
        DataWriter(Database& db) : db_{db} {}

        void AddBus(Bus&& bus) const override;

        void AddBus(std::string&& name, const std::vector<std::string_view>& stops, bool is_roundtrip) const override;

        void AddBus(std::string&& name, std::vector<std::string>&& stops, bool is_roundtrip) const override;

        void AddStop(Stop&& stop) const override;

        void AddStop(std::string&& name, Coordinates&& coordinates) const override;

        void SetMeasuredDistance(const std::string_view from_stop_name, const std::string_view to_stop_name, double distance) const override;

        void SetMeasuredDistance(data::MeasuredRoadDistance&& distance) const override;

    private:
        Database& db_;
    };

    template <class Owner>
    class Database<Owner>::DataReader : public ITransportDataReader {
    public:
        DataReader(Database& db) : db_{db} {}

        BusRecord GetBus(std::string_view name) const override;

        StopRecord GetStop(std::string_view name) const override;

        const DatabaseScheme::StopsTable& GetStopsTable() const override {
            return db_.GetStopsTable();
        }

        const DatabaseScheme::BusRoutesTable& GetBusRoutesTable() const override {
            return db_.GetBusRoutesTable();
        }

        const BusRecordSet& GetBuses(StopRecord stop) const override;

        const BusRecordSet& GetBuses(const std::string_view bus_name) const override;

        DistanceBetweenStopsRecord GetDistanceBetweenStops(StopRecord from, StopRecord to) const override;

    private:
        Database& db_;
    };
}

namespace transport_catalogue::data /* Database implementation */ {

    template <class Owner>
    template <typename Stop, detail::EnableIfSame<Stop, data::Stop>>
    const Stop& Database<Owner>::AddStop(Stop&& stop) {
        assert(name_to_stop_.count(stop.name) == 0);

        const Stop& new_stop = stops_.emplace_back(std::forward<Stop>(stop));
        name_to_stop_[new_stop.name] = &new_stop;
        return new_stop;
    }

    template <class Owner>
    template <
        typename String, typename Coordinates,
        detail::EnableIf<detail::IsSameV<String, std::string> && detail::IsConvertibleV<Coordinates, data::Coordinates>>>
    const Stop& Database<Owner>::AddStop(String&& name, Coordinates&& coordinates) {
        return AddStop(Stop{std::move(name), std::move(coordinates)});
    }

    template <class Owner>
    void Database<Owner>::AddMeasuredDistance(const std::string_view from_stop_name, const std::string_view to_stop_name, double distance) {
        const Stop* from_stop = GetStop(from_stop_name);
        const Stop* to_stop = GetStop(to_stop_name);
        assert(from_stop != nullptr && to_stop != nullptr);

        double pseudo_length = geo::ComputeDistance(from_stop->coordinates, to_stop->coordinates);

        measured_distances_btw_stops_[{from_stop, to_stop}] = {pseudo_length, distance};
    }

    template <class Owner>
    template <typename Bus, detail::EnableIfSame<Bus, data::Bus>>
    const Bus& Database<Owner>::AddBus(Bus&& bus) {
        assert(name_to_bus_.count(bus.name) == 0);

        const Bus& new_bus = bus_routes_.emplace_back(std::forward<Bus>(bus));
        name_to_bus_[new_bus.name] = &new_bus;
        std::for_each(new_bus.route.begin(), new_bus.route.end(), [this, &new_bus](const Stop* stop) {
            stop_to_buses_[stop].insert(&new_bus);
        });
        return new_bus;
    }

    template <class Owner>
    template <typename String, typename Route, detail::EnableIf<detail::IsSameV<String, std::string> && detail::IsSameV<Route, data::Route>>>
    const Bus& Database<Owner>::AddBus(String&& name, Route&& route, bool is_roundtrip) {
        return AddBus(Bus{std::forward<String>(name), std::forward<Route>(route), is_roundtrip});
    }

    template <class Owner>
    template <
        typename String, typename StopsNameContainer,
        detail::EnableIf<
            detail::IsConvertibleV<String, std::string> &&
            (detail::IsSameV<StopsNameContainer, std::vector<std::string>> || detail::IsSameV<StopsNameContainer, std::vector<std::string_view>>)>>
    const Bus& Database<Owner>::AddBus(String&& name, StopsNameContainer&& stops, bool is_roundtrip) {
        Route route = ToRoute(std::forward<StopsNameContainer>(stops));
        return AddBus(std::forward<String>(name), std::move(route), is_roundtrip);
    }

    template <class Owner>
    template <
        typename String, typename StopsNameContainer,
        detail::EnableIf<detail::IsConvertibleV<String, std::string> && detail::IsSameV<StopsNameContainer, std::vector<std::string_view>>>>
    const Bus* Database<Owner>::AddBusForce(String&& name, StopsNameContainer&& stops, bool is_roundtrip) {
        Route route;
        route.reserve(stops.size());
        std::for_each(stops.begin(), stops.end(), [&](const std::string_view stop) {
            auto ptr = name_to_stop_.find(stop);
            if (ptr != name_to_stop_.end()) {
                route.push_back(ptr->second);
            }
        });

        return AddBus(std::forward<String>(name), std::move(route), is_roundtrip);
    }

    template <class Owner>
    template <
        typename Container,
        detail::EnableIf<detail::IsSameV<Container, std::vector<std::string_view>> || detail::IsSameV<Container, std::vector<std::string>>>>
    Route Database<Owner>::ToRoute(Container&& stops) const {
        Route route{stops.size()};
        std::transform(stops.begin(), stops.end(), route.begin(), [this](const std::string_view stop) {
            assert(name_to_stop_.count(stop));
            return name_to_stop_.at(stop);
        });
        return route;
    }

    template <class Owner>
    template <
        typename StringView, typename TableView,
        detail::EnableIf<detail::IsConvertibleV<StringView, std::string_view> && detail::IsBaseOfV<TableView, TableView>>>
    const auto* Database<Owner>::GetItem(StringView&& name, const TableView& table) const {
        auto ptr = table.find(std::move(name));
        return ptr == table.end() ? nullptr : ptr->second;
    }

    template <class Owner>
    const Bus* Database<Owner>::GetBus(const std::string_view name) const {
        const Bus* result = GetItem(std::move(name), name_to_bus_);
        return result;
    }

    template <class Owner>
    const Stop* Database<Owner>::GetStop(const std::string_view name) const {
        return GetItem(std::move(name), name_to_stop_);
    }

    template <class Owner>
    const DatabaseScheme::StopsTable& Database<Owner>::GetStopsTable() const {
        return stops_;
    }

    template <class Owner>
    const DatabaseScheme::BusRoutesTable& Database<Owner>::GetBusRoutesTable() const {
        return bus_routes_;
    }

    template <class Owner>
    const DatabaseScheme::NameToStopView& Database<Owner>::GetNameToStopView() const {
        return name_to_stop_;
    }

    template <class Owner>
    const ITransportDataWriter& Database<Owner>::GetDataWriter() const {
        return db_writer_;
    }

    template <class Owner>
    const ITransportDataReader& Database<Owner>::GetDataReader() const {
        return db_reader_;
    }

    template <class Owner>
    void Database<Owner>::LockDatabase() {
        throw transport_catalogue::exceptions::NotImplementedException();
        mutex_.lock();
    }

    template <class Owner>
    void Database<Owner>::UnlockDatabase() {
        throw transport_catalogue::exceptions::NotImplementedException();
        mutex_.unlock();
    }

    template <class Owner>
    std::lock_guard<std::mutex> Database<Owner>::LockGuard() {
        throw transport_catalogue::exceptions::NotImplementedException();
        return std::lock_guard<std::mutex>{mutex_};
    }
}

namespace transport_catalogue::data /* Database::DataWriter implementation */ {

    template <class Owner>
    void Database<Owner>::DataWriter::AddBus(Bus&& bus) const {
        db_.AddBus(std::move(bus));
    }

    template <class Owner>
    void Database<Owner>::DataWriter::AddBus(std::string&& name, const std::vector<std::string_view>& stops, bool is_roundtrip) const {
        db_.AddBus(std::move(name), stops, is_roundtrip);
    }

    template <class Owner>
    void Database<Owner>::DataWriter::AddBus(std::string&& name, std::vector<std::string>&& stops, bool is_roundtrip) const {
        db_.AddBus(std::move(name), std::move(stops), is_roundtrip);
    }

    template <class Owner>
    void Database<Owner>::DataWriter::AddStop(Stop&& stop) const {
        db_.AddStop(std::move(stop));
    }

    template <class Owner>
    void Database<Owner>::DataWriter::AddStop(std::string&& name, Coordinates&& coordinates) const {
        db_.AddStop(std::move(name), std::move(coordinates));
    }

    template <class Owner>
    void Database<Owner>::DataWriter::SetMeasuredDistance(
        const std::string_view from_stop_name, const std::string_view to_stop_name, double distance) const {
        db_.AddMeasuredDistance(from_stop_name, to_stop_name, distance);
    }

    template <class Owner>
    void Database<Owner>::DataWriter::SetMeasuredDistance(data::MeasuredRoadDistance&& distance) const {
        db_.AddMeasuredDistance(std::move(distance.from_stop), std::move(distance.to_stop), std::move(distance.distance));
    }
}

namespace transport_catalogue::data /* Database::DataReader implementation */ {

    template <class Owner>
    BusRecord Database<Owner>::DataReader::GetBus(std::string_view name) const {
        return db_.GetBus(name);
    }

    template <class Owner>
    StopRecord Database<Owner>::DataReader::GetStop(std::string_view name) const {
        return db_.GetStop(name);
    }

    template <class Owner>
    const BusRecordSet& Database<Owner>::DataReader::GetBuses(StopRecord stop) const {
        static const BusRecordSet empty_result;
        auto ptr = db_.stop_to_buses_.find(stop);
        return ptr == db_.stop_to_buses_.end() ? empty_result : ptr->second;
    }

    template <class Owner>
    const BusRecordSet& Database<Owner>::DataReader::GetBuses(const std::string_view bus_name) const {
        static const BusRecordSet empty_result;
        auto stop_ptr = GetStop(bus_name);
        return stop_ptr == nullptr ? empty_result : GetBuses(stop_ptr);
    }

    template <class Owner>
    DistanceBetweenStopsRecord Database<Owner>::DataReader::GetDistanceBetweenStops(StopRecord from, StopRecord to) const {
        auto ptr = db_.measured_distances_btw_stops_.find({from, to});
        if (ptr != db_.measured_distances_btw_stops_.end()) {
            return ptr->second;
        } else if (ptr = db_.measured_distances_btw_stops_.find({to, from}); ptr != db_.measured_distances_btw_stops_.end()) {
            return ptr->second;
        }
        return {0., 0.};
    }
}