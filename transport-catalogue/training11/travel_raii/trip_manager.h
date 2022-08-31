#pragma once
#include <string>
#include <vector>

#include "flight_provider.h"
#include "hotel_provider.h"

class Trip {
    HotelProvider* hp_;
    FlightProvider* fp_;

public:
    std::vector<HotelProvider::BookingId> hotels;
    std::vector<FlightProvider::BookingId> flights;

    Trip(HotelProvider& hp, FlightProvider& fp) : hp_(&hp), fp_(&fp){};

    Trip(const Trip& other) {
        FlightProvider fp;
        HotelProvider hp;
        Trip temp(hp, fp);
        temp.flights = other.flights;
        temp.hotels = other.hotels;
        temp.hp_ = other.hp_;
        temp.fp_ = other.fp_;
        Swap(temp);
    }

    Trip(Trip&& other) noexcept {
        FlightProvider fp;
        HotelProvider hp;
        Trip temp(hp, fp);
        temp.flights = std::move(other.flights);
        temp.hotels = std::move(other.hotels);
        temp.hp_ = std::move(other.hp_);
        temp.fp_ = std::move(other.fp_);
        Swap(temp);
    }

    Trip& operator=(const Trip& other) {
        Trip temp(other);
        Swap(temp);
        return *this;
    }

    Trip& operator=(Trip&& other) noexcept {
        Trip temp(std::move(other));
        Swap(temp);
        return *this;
    }

    void Swap(Trip& other) {
        std::swap(this->flights, other.flights);
        std::swap(this->hotels, other.hotels);
        std::swap(this->hp_, other.hp_);
        std::swap(this->fp_, other.fp_);
    }

    void Cancel() {
        for (const HotelProvider::BookingId id : hotels) {
            hp_->Cancel(id);
        }
        hotels.clear();
        for (const FlightProvider::BookingId id : flights) {
            fp_->Cancel(id);
        }
        flights.clear();
    }

    ~Trip() {
        Cancel();
    }
};

class TripManager {
public:
    using BookingId = std::string;
    struct BookingData {
        std::string city_from;
        std::string city_to;
        std::string date_from;
        std::string date_to;
    };

    Trip Book(const BookingData& data) {
        Trip trip(hotel_provider_, flight_provider_);
        {
            FlightProvider::BookingData flight_booking_data{data.city_from, data.city_to, data.date_from};
            trip.flights.push_back(flight_provider_.Book(flight_booking_data));
        }
        {
            HotelProvider::BookingData hotel_booking_data{data.city_from, data.date_from, data.date_to};
            trip.hotels.push_back(hotel_provider_.Book(hotel_booking_data));
        }
        {
            FlightProvider::BookingData flight_booking_data{data.city_from, data.city_to, data.date_from};
            trip.flights.push_back(flight_provider_.Book(flight_booking_data));
        }
        return trip;
    }

    void Cancel(Trip& trip) {
        trip.Cancel();
    }

private:
    HotelProvider hotel_provider_;
    FlightProvider flight_provider_;
};