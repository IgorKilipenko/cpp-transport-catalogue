#pragma once

/*
 * В этом файле вы можете разместить классы/структуры, которые являются частью предметной области (domain)
 * вашего приложения и не зависят от транспортного справочника. Например Автобусные маршруты и Остановки.
 *
 * Их можно было бы разместить и в transport_catalogue.h, однако вынесение их в отдельный
 * заголовочный файл может оказаться полезным, когда дело дойдёт до визуализации карты маршрутов:
 * визуализатор карты (map_renderer) можно будет сделать независящим от транспортного справочника.
 *
 * Если структура вашего приложения не позволяет так сделать, просто оставьте этот файл пустым.
 *
 */

#include <algorithm>
#include <cassert>
#include <deque>
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
#include <vector>

#include "geo.h"

namespace transport_catalogue::detail {
    template <bool Condition>
    using EnableIf = typename std::enable_if_t<Condition, bool>;

    template <typename FromType, typename ToType>
    using EnableIfConvertible = std::enable_if_t<std::is_convertible_v<std::decay_t<FromType>, ToType>, bool>;

    template <typename FromType, typename ToType>
    using IsConvertible = std::is_convertible<std::decay_t<FromType>, ToType>;

    template <typename FromType, typename ToType>
    inline constexpr bool IsConvertibleV = std::is_convertible<std::decay_t<FromType>, ToType>::value;

    template <typename FromType, typename ToType>
    using IsSame = std::is_same<std::decay_t<FromType>, ToType>;

    template <typename FromType, typename ToType>
    inline constexpr bool IsSameV = std::is_same<std::decay_t<FromType>, ToType>::value;

    template <typename FromType, typename ToType>
    using EnableIfSame = std::enable_if_t<std::is_same_v<std::decay_t<FromType>, ToType>, bool>;

    template <typename BaseType, typename DerivedType>
    using IsBaseOf = std::is_base_of<BaseType, std::decay_t<DerivedType>>;

    template <typename FromType, typename ToType>
    inline constexpr bool IsBaseOfV = IsBaseOf<FromType, ToType>::value;

    template <typename BaseType, typename DerivedType>
    using EnableIfBaseOf = std::enable_if_t<std::is_base_of_v<BaseType, std::decay_t<DerivedType>>, bool>;
}

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

namespace transport_catalogue::data {
    using Coordinates = geo::Coordinates;

    struct Stop {
        std::string name;
        Coordinates coordinates;
        Stop() = default;
        template <
            typename String = std::string, typename Coordinates = data::Coordinates,
            detail::EnableIf<detail::IsConvertibleV<String, std::string_view> && detail::IsSameV<Coordinates, data::Coordinates>> = true>
        Stop(String&& name, Coordinates&& coordinates) : name{std::forward<String>(name)}, coordinates{std::forward<Coordinates>(coordinates)} {}
        // Stop(Stop&& other) : name{std::move(other.name)}, coordinates{std::move(other.coordinates)} {}

        bool operator==(const Stop& rhs) const noexcept {
            return this == &rhs || (name == rhs.name && coordinates == rhs.coordinates);
        }

        bool operator!=(const Stop& rhs) const noexcept {
            return !(*this == rhs);
        }
    };

    using StopRecord = const Stop*;
    using StopRecordSet = std::deque<StopRecord>;
    
    class Route : public std::vector<StopRecord> {
        using vector::vector;
    };

    struct Bus {
        std::string name;
        Route route;
        Bus() = default;
        template <
            typename String = std::string, typename Route = data::Route,
            detail::EnableIf<detail::IsConvertibleV<String, std::string_view> && detail::IsSameV<Route, data::Route>> = true>
        Bus(String&& name, Route&& route) : name{std::forward<String>(name)}, route{std::forward<Route>(route)} {}
        // Bus(Bus&& other) : name{std::move(other.name)}, route{std::move(other.route)} {}

        bool operator==(const Bus& rhs) const noexcept {
            return this == &rhs || (name == rhs.name && route == rhs.route);
        }

        bool operator!=(const Bus& rhs) const noexcept {
            return !(*this == rhs);
        }
    };

    using BusRecord = const Bus*;
    using BusRecordSet = std::set<BusRecord>;

    struct DistanceBetweenStopsRecord {
        double distance = 0.;
        double measured_distance = 0.;
    };

    struct BusStat {
        size_t total_stops{};
        size_t unique_stops{};
        double route_length{};
        double route_curvature{};
    };

    class Hasher {
    public:
        size_t operator()(const std::pair<const Stop*, const Stop*>& stops) const {
            return this->operator()({stops.first, stops.second});
        }

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

    class ITransportDataReader {
    public:
        virtual BusRecord GetBus(std::string_view name) const = 0;
        virtual StopRecord GetStop(std::string_view name) const = 0;
        virtual const BusRecordSet& GetBuses(StopRecord stop) const = 0;
        virtual DistanceBetweenStopsRecord GetDistanceBetweenStops(StopRecord from, StopRecord to) const = 0;

        virtual ~ITransportDataReader() = default;
    };

    class ITransportDataWriter {
    public:
        virtual void AddBus(Bus&& bus) const = 0;
        virtual void AddBus(std::string_view name, const std::vector<std::string_view>& stops) const = 0;

        virtual void AddStop(Stop&& stop) const = 0;
        virtual void AddStop(std::string_view, Coordinates&& coordinates) const = 0;

        virtual void SetMeasuredDistance(const std::string_view from_stop_name, const std::string_view to_stop_name, double distance) const = 0;

        virtual ~ITransportDataWriter() = default;
    };

    class ITransportStatDataReader {
    public:
        virtual const BusStat GetBusInfo(const data::Bus* bus) const = 0;
        virtual const std::optional<BusStat> GetBusInfo(const std::string_view bus_name) const = 0;
        virtual const data::ITransportDataReader& GetDataReader() const = 0;

        virtual ~ITransportStatDataReader() = default;
    };

    template <class Owner>
    class Database /*: public ITransportDataReader, public ITransportDataWriter*/ {
        friend Owner;

    private: /* aliases */
        using StopsTableBase = std::deque<Stop>;
        using BusRoutesTableBase = std::deque<Bus>;
        using DistanceBetweenStopsTableBase = std::unordered_map<std::pair<const Stop*, const Stop*>, DistanceBetweenStopsRecord, Hasher>;
        using NameToStopViewBase = std::unordered_map<std::string_view, const data::Stop*>;
        using NameToBusRoutesViewBase = std::unordered_map<std::string_view, const data::Bus*>;
        // using StopToBusesViewBase = std::unordered_map<const Stop*, std::set<std::string_view, std::less<>>>;
        using StopToBusesViewBase = std::unordered_map<StopRecord, BusRecordSet>;

    public: /* DB scheme */
        class DataTable {};
        class TableView {};

        class StopsTable : public DataTable, public StopsTableBase {
            using StopsTableBase::deque;
        };

        class BusRoutesTable : public DataTable, public BusRoutesTableBase {
            using BusRoutesTableBase::deque;
        };

        class DistanceBetweenStopsTable : public DataTable, public DistanceBetweenStopsTableBase {
            using DistanceBetweenStopsTableBase::unordered_map;
        };

        class NameToStopView : public TableView, public NameToStopViewBase {
            using NameToStopViewBase::unordered_map;
        };

        class NameToBusRoutesView : public TableView, public NameToBusRoutesViewBase {
            using NameToBusRoutesViewBase::unordered_map;
        };

        class StopToBusesView : public TableView, public StopToBusesViewBase {
            using StopToBusesViewBase::unordered_map;
        };

    public:
        Database() : db_writer_{*this}, db_reader_{*this} {}

        const StopsTable& GetStopsTable() const;

        const BusRoutesTable& GetBusRoutesTable() const;

        const NameToStopView& GetNameToStopView() const;

        const ITransportDataWriter& GetDataWriter() const {
            return db_writer_;
        }

        const ITransportDataReader& GetDataReader() const {
            return db_reader_;
        }

        explicit operator ITransportDataWriter() const {
            return db_writer_;
        }

        explicit operator ITransportDataReader() const {
            return db_reader_;
        }

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
            detail::EnableIf<detail::IsSameV<String, std::string> && detail::IsConvertibleV<Route, data::Route>> = true>
        const Bus& AddBus(String&& name, Route&& route);

        template <
            typename String = std::string, typename StopsNameContainer = std::vector<std::string_view>,
            detail::EnableIf<detail::IsConvertibleV<String, std::string> && detail::IsSameV<StopsNameContainer, std::vector<std::string_view>>> =
                true>
        const Bus& AddBus(String&& name, StopsNameContainer&& route);

        template <
            typename String = std::string, typename StopsNameContainer = std::vector<std::string_view>,
            detail::EnableIf<detail::IsConvertibleV<String, std::string> && detail::IsSameV<StopsNameContainer, std::vector<std::string_view>>> =
                true>
        const Bus* AddBusForce(String&& name, StopsNameContainer&& stops) {
            Route route;
            route.reserve(stops.size());
            std::for_each(stops.begin(), stops.end(), [&](const std::string_view stop) {
                auto ptr = name_to_stop_.find(stop);
                if (ptr != name_to_stop_.end()) {
                    route.push_back(ptr->second);
                }
            });

            return AddBus(std::forward<String>(name), std::move(route));
        }

        const Bus* GetBus(const std::string_view name) const;

        const Stop* GetStop(const std::string_view name) const;

    private:
        StopsTable stops_;
        BusRoutesTable bus_routes_;
        DistanceBetweenStopsTable measured_distances_btw_stops_;

        NameToStopView name_to_stop_;
        NameToBusRoutesView name_to_bus_;
        StopToBusesView stop_to_buses_;

        std::mutex mutex_;

        template <
            typename StringView, typename TableView,
            detail::EnableIf<detail::IsConvertibleV<StringView, std::string_view> && detail::IsBaseOfV<Database::TableView, TableView>> = true>
        const auto* GetItem(StringView&& name, const TableView& table) const;

        void LockDatabase() {
            throw transport_catalogue::exceptions::NotImplementedException();
            mutex_.lock();
        }

        void UnlockDatabase() {
            throw transport_catalogue::exceptions::NotImplementedException();
            mutex_.unlock();
        }

        std::lock_guard<std::mutex> LockGuard() {
            throw transport_catalogue::exceptions::NotImplementedException();
            return std::lock_guard<std::mutex>{mutex_};
        }

    public:
        class DataWriter : public ITransportDataWriter {
        public:
            DataWriter(Database& db) : db_{db} {}

            void AddBus(Bus&& bus) const override {
                db_.AddBus(std::move(bus));
            }

            void AddBus(std::string_view name, const std::vector<std::string_view>& stops) const override {
                db_.AddBus(static_cast<std::string>(name), stops);
            }

            void AddStop(Stop&& stop) const override {
                db_.AddStop(std::move(stop));
            }

            void AddStop(std::string_view name, Coordinates&& coordinates) const override {
                db_.AddStop(static_cast<std::string>(name), coordinates);
            }

            void SetMeasuredDistance(const std::string_view from_stop_name, const std::string_view to_stop_name, double distance) const override {
                db_.AddMeasuredDistance(from_stop_name, to_stop_name, distance);
            }

        private:
            Database& db_;
        };

        class DataReader : public ITransportDataReader {
        public:
            DataReader(Database& db) : db_{db} {}

            BusRecord GetBus(std::string_view name) const override {
                return db_.GetBus(name);
            }

            StopRecord GetStop(std::string_view name) const override {
                return db_.GetStop(name);
            }

            virtual const BusRecordSet& GetBuses(StopRecord stop) const override {
                static const std::set<BusRecord> empty_result;
                auto ptr = db_.stop_to_buses_.find(stop);
                return ptr == db_.stop_to_buses_.end() ? empty_result : ptr->second;
            }

            DistanceBetweenStopsRecord GetDistanceBetweenStops(StopRecord from, StopRecord to) const override {
                auto ptr = db_.measured_distances_btw_stops_.find({from, to});
                if (ptr != db_.measured_distances_btw_stops_.end()) {
                    return ptr->second;
                } else if (ptr = db_.measured_distances_btw_stops_.find({to, from}); ptr != db_.measured_distances_btw_stops_.end()) {
                    return ptr->second;
                }
                return {0., 0.};
            }

        private:
            Database& db_;
        };

    private:
        DataWriter db_writer_;
        DataReader db_reader_;
    };
}

namespace transport_catalogue::data {

    template <class Owner>
    template <typename Stop, detail::EnableIfSame<Stop, data::Stop>>
    const Stop& Database<Owner>::AddStop(Stop&& stop) {
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

        measured_distances_btw_stops_[{from_stop, to_stop}] = {distance, pseudo_length};
    }

    template <class Owner>
    template <typename Bus, detail::EnableIfSame<Bus, data::Bus>>
    const Bus& Database<Owner>::AddBus(Bus&& bus) {
        const Bus& new_bus = bus_routes_.emplace_back(std::forward<Bus>(bus));
        name_to_bus_[new_bus.name] = &new_bus;
        std::for_each(new_bus.route.begin(), new_bus.route.end(), [this, &new_bus](const Stop* stop) {
            stop_to_buses_[stop].insert(&new_bus);
        });
        return new_bus;
    }

    template <class Owner>
    template <typename String, typename Route, detail::EnableIf<detail::IsSameV<String, std::string> && detail::IsConvertibleV<Route, data::Route>>>
    const Bus& Database<Owner>::AddBus(String&& name, Route&& route) {
        return AddBus(Bus{std::forward<String>(name), std::forward<Route>(route)});
    }

    template <class Owner>
    template <
        typename String, typename StopsNameContainer,
        detail::EnableIf<detail::IsConvertibleV<String, std::string> && detail::IsSameV<StopsNameContainer, std::vector<std::string_view>>>>
    const Bus& Database<Owner>::AddBus(String&& name, StopsNameContainer&& stops) {
        Route route{stops.size()};
        std::transform(stops.begin(), stops.end(), route.begin(), [&](const std::string_view stop) {
            assert(name_to_stop_.count(stop));
            return name_to_stop_[stop];
        });

        return AddBus(std::forward<String>(name), std::move(route));
    }

    template <class Owner>
    template <
        typename StringView, typename TableView,
        detail::EnableIf<detail::IsConvertibleV<StringView, std::string_view> && detail::IsBaseOfV<typename Database<Owner>::TableView, TableView>>>
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
    const typename Database<Owner>::StopsTable& Database<Owner>::GetStopsTable() const {
        return stops_;
    }

    template <class Owner>
    const typename Database<Owner>::BusRoutesTable& Database<Owner>::GetBusRoutesTable() const {
        return bus_routes_;
    }

    template <class Owner>
    const typename Database<Owner>::NameToStopView& Database<Owner>::GetNameToStopView() const {
        return name_to_stop_;
    }
}