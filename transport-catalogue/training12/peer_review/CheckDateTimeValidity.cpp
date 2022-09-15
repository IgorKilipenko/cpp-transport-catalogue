#include <array>
#include <exception>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>

struct Date {
    uint16_t year;
    uint8_t month;
    uint8_t day;
};

struct Time {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

struct DateTime : public Date, public Time {};

template <typename ErrorType_, std::enable_if_t<std::is_base_of_v<std::exception, ErrorType_>, bool> = true>
using ErrorValue = std::optional<ErrorType_>;

[[nodiscard]] ErrorValue<std::domain_error> CheckDateValidity(const Date& dt) {
    using namespace std::string_literals;

    if (dt.year < 1) {
        return std::domain_error("year is too small"s);
    }
    if (dt.year > 9999) {
        return std::domain_error("year is too big"s);
    }

    if (dt.month < 1) {
        return std::domain_error("month is too small"s);
    }
    if (dt.month > 12) {
        return std::domain_error("month is too big"s);
    }

    const bool is_leap_year = (dt.year % 4 == 0) && !(dt.year % 100 == 0 && dt.year % 400 != 0);
    const std::array month_lengths{31, 28 + is_leap_year, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (dt.day < 1) {
        return std::domain_error("day is too small"s);
    }
    if (dt.day > month_lengths[dt.month - 1]) {
        return std::domain_error("day is too big"s);
    }

    return std::nullopt;
}

[[nodiscard]] ErrorValue<std::domain_error> CheckTimeValidity(const Time& dt) {
    using namespace std::string_literals;

    if (dt.hour > 23) {
        throw std::domain_error("hour is too big"s);
    }

    if (dt.minute > 59) {
        throw std::domain_error("minute is too big"s);
    }

    if (dt.second > 59) {
        throw std::domain_error("second is too big"s);
    }

    return std::nullopt;
}

[[nodiscard]] ErrorValue<std::domain_error> CheckDateTimeValidity(const DateTime& dt) {
    ErrorValue<std::domain_error> err;
    if (err = CheckDateValidity(dt); err.has_value()) {
        return err;
    }
    if (err = CheckTimeValidity(dt); err.has_value()) {
        return err;
    }

    return std::nullopt;
}