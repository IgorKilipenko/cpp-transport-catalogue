#pragma once

#include <cstddef>
#include <regex>
#include <string>
#include <string_view>

class Date {
public:
    Date() {}
    Date(int year, int month, int day) : year_(year), month_(month), day_(day) {}

    Date operator+(const Date& other) const {
        return Date{year_ + other.year_, month_ + other.month_, day_ + other.day_};
    }

    Date& operator++() {
        ++day_;
        return *this;
    }

    bool operator<(const Date& other) const {
        if (this == &other || year_ > other.year_) {
            return false;
        }

        // return day_ * 86400ul + month_ * 2629746ul + year_ * 31556952ul < other.day_ * 86400ul + other.month_ * 2629746ul + other.year_ *
        // 31556952ul;
        return (static_cast<int64_t>(day_) - static_cast<int64_t>(other.day_)) * 86400l +
                   (static_cast<int64_t>(month_) - static_cast<int64_t>(other.month_)) * 2629746l + (static_cast<int64_t>(year_) - static_cast<int64_t>(other.year_)) * 31556952l <
               0l;

        //return ComputeDistance(*this, other) > 0;
    }

    bool operator==(const Date& other) const {
        return this == &other || (day_ == other.day_ && month_ == other.month_ && year_ == other.year_);
    }

    bool operator!=(const Date& other) const {
        return !(*this == other);
    }

    std::string ToString() const {
        return std::string(std::to_string(day_) + "." + std::to_string(month_) + "." + std::to_string(year_));
    }

    // тут правильно использовать string_view, но regex его пока не поддерживает
    static Date FromString(const std::string& str) {
        static const std::regex date_regex(R"/(([0-9]{4})-([0-9]{2})-([0-9]{2}))/");
        std::smatch m;

        if (!std::regex_match(str, m, date_regex)) {
            return {};
        }

        return Date(std::stoi(m[1].str()), std::stoi(m[2].str()), std::stoi(m[3].str()));
    }

    static Date FromString(std::string_view str) {
        return FromString(std::string(str));
    }

    static int ComputeDistance(Date from, Date to) {
        auto date_to_seconds = [](Date date) {
            tm timestamp;
            timestamp.tm_sec = 0;
            timestamp.tm_min = 0;
            timestamp.tm_hour = 0;
            timestamp.tm_mday = date.day_;
            timestamp.tm_mon = date.month_ - 1;
            timestamp.tm_year = date.year_ - 1900;
            timestamp.tm_isdst = 0;
            return mktime(&timestamp);
        };

        static constexpr int SECONDS_IN_DAY = 60 * 60 * 24;
        return static_cast<int>((date_to_seconds(to) - date_to_seconds(from)) / SECONDS_IN_DAY);
    }

private:
    int year_ = 0, month_ = 0, day_ = 0;
};
