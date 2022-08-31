#pragma once
#include <algorithm>

namespace raii {
    template <typename Provider>
    class Booking {
    private:
        using BookingId = typename Provider::BookingId;

        Provider* provider_ = nullptr;
        int* count_ = nullptr;

    public:
        Booking() = default;
        Booking(Provider* p, int& count) : provider_(p), count_(&count) {};

        Booking(const Booking&);
        Booking(Booking&& other);

        Booking& operator=(const Booking&);
        Booking& operator=(Booking&& other);

        ~Booking();

        BookingId GetId() const {
            return count_;
        };

    private:
        void Swap(Booking&);
    };
    template<typename Provider>
    void Booking<Provider>::Swap(Booking& other) {
        std::swap(count_, other.count_);
        std::swap(provider_, other.provider_);
    }

    template<typename Provider>
    Booking<Provider>::Booking(const Booking& other) {
        Booking temp;
        temp.count_ = other.count_;
        temp.provider_ = other.provider_;
        Swap(temp);
    }

    template<typename Provider>
    Booking<Provider>::Booking(Booking&& other) {
        Booking temp;
        temp.count_ = std::move(other.count_);
        temp.provider_ = std::move(other.provider_);
        Swap(temp);
    }

    template<typename Provider>
    Booking<Provider>& Booking<Provider>::operator=(const Booking& other) {
        Booking temp;
        temp.count_ = other.count_;
        temp.provider_ = other.provider_;
        Swap(temp);
        return *this;
    }

    template<typename Provider>
    Booking<Provider>& Booking<Provider>::operator=(Booking&& other) {
        Booking temp;
        temp.count_ = std::move(other.count_);
        temp.provider_ = std::move(other.provider_);
        Swap(temp);
        return *this;
    }

    template<typename Provider>
    Booking<Provider>::~Booking() {
        provider_ = nullptr;
        if (count_ && *count_ > 0) {
            --*count_;            
        }
        count_ = nullptr;
    }
}