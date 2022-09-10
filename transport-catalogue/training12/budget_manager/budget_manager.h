#pragma once
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <ostream>

#include "date.h"

namespace budget_manager {
    class BudgetManager {
    public:
        inline static const Date FirstDate{2000, 1, 1};
        inline static const Date LastDate{2100, 1, 1};

        BudgetManager(std::ostream& out_stream) : out_stream_(out_stream), earnings_(Date::ComputeDistance(FirstDate, LastDate)) {}

        inline static const Date START_DATE{2000, 1, 1};
        inline static const Date END_DATE{2100, 1, 1};
        inline static const double TAX_RATE = 0.13;

        // разработайте класс BudgetManager
        double ComputeIncome(Date from, Date to) const {
            double result = 0.;
            auto begin = earnings_.begin() + Date::ComputeDistance(FirstDate, from);
            auto end = begin + Date::ComputeDistance(from, to) + 1;

            std::for_each(begin, end, [&result](const auto& ebt_item) {
                auto& [ebt, earn] = ebt_item;
                result += ebt + earn;
            });

            out_stream_ << result << std::endl;
            return result;
        }

        void Earn(Date from, Date to, double value) {
            size_t size = Date::ComputeDistance(from, to) + 1ul;
            auto begin = earnings_.begin() + Date::ComputeDistance(FirstDate, from);
            auto end = begin + size;
            std::for_each(begin, end, [value, size](auto& ebt) {
                ebt.first += value / size;
            });
        }

        void PayTax(Date from, Date to) {
            auto begin = earnings_.begin() + Date::ComputeDistance(FirstDate, from);
            auto end = begin + Date::ComputeDistance(from, to) + 1;
            std::for_each(begin, end, [](auto& ebt_item) {
                auto& [ebt, tax_free] = ebt_item;
                tax_free = (ebt + tax_free) * (1. - TAX_RATE);
                ebt = 0.;
            });
        }

    private:
        std::ostream& out_stream_;
        std::vector<std::pair<double, double>> earnings_;
    };
}
