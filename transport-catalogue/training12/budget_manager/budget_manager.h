#pragma once
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <ostream>
#include <type_traits>
#include <utility>

#include "date.h"

namespace budget_manager {
    class BudgetManager {
    private:
        struct BudgetSpane {
            double earning_before_taxes = 0.0;
            double net_income = 0.0;
        };

    public:
        inline static const Date FirstDate{2000, 1, 1};
        inline static const Date LastDate{2100, 1, 1};
        inline static const Date START_DATE{2000, 1, 1};
        inline static const Date END_DATE{2100, 1, 1};
        inline static const double TAX_RATE = 0.13;

        BudgetManager(std::ostream& out_stream) : out_stream_(out_stream), earnings_(Date::ComputeDistance(FirstDate, LastDate)) {}

        double ComputeIncome(Date from, Date to) const {
            double result = 0.;
            //! Const_cast used to avoid duplicating GetRange_ code for const object.
            auto [begin, end] = const_cast<BudgetManager*>(this)->GetRange_(std::move(from), std::move(to));

            std::for_each(begin, end, [&result](const /* !span item must be const value - do't edit */ BudgetSpane& span) {
                result += span.earning_before_taxes + span.net_income;
            });

            out_stream_ << result << std::endl;
            return result;
        }

        void Earn(Date from, Date to, double value) {
            auto [begin, end] = GetRange_(std::move(from), std::move(to));
            size_t size = end - begin;
            std::for_each(begin, end, [value, size](BudgetSpane& span) {
                span.earning_before_taxes += value / size;
            });
        }

        void PayTax(Date from, Date to, double tax_rate = TAX_RATE) {
            auto [begin, end] = GetRange_(std::move(from), std::move(to));
            std::for_each(begin, end, [tax_rate](BudgetSpane& span) {
                span.net_income = (span.earning_before_taxes + span.net_income) * (1. - tax_rate);
                span.earning_before_taxes = 0.;
            });
        }

    private:
        std::ostream& out_stream_;
        std::vector<BudgetSpane> earnings_;

        template <typename DateType_, std::enable_if_t<std::is_same_v<std::decay_t<DateType_>, Date>, bool> = true>
        std::pair<decltype(earnings_)::iterator, decltype(earnings_)::iterator> GetRange_(DateType_&& from, DateType_&& to) {
            auto&& begin = earnings_.begin() + Date::ComputeDistance(FirstDate, from);
            auto&& end = begin + Date::ComputeDistance(std::forward<DateType_>(from), std::forward<DateType_>(to)) + 1ul;
            return {std::move(begin), std::move(end)};
        }
    };
}
