#pragma once
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <ostream>

#include "date.h"

namespace budget_manager {
    class BudgetManager {
    public:
        BudgetManager(std::ostream& out_stream) : out_stream_(out_stream) {}

        inline static const Date START_DATE{2000, 1, 1};
        inline static const Date END_DATE{2100, 1, 1};
        inline static const double TAX_RATE = 0.13;

        // разработайте класс BudgetManager
        double ComputeIncome(Date from, Date to) const {
            double result = 0.;
            auto begin = earnings_before_taxes_.lower_bound(from);
            //std::cerr << "begin: " << begin->first.ToString() << std::endl;
            auto end = earnings_before_taxes_.upper_bound(to);

            std::for_each(begin, end, [&result](const auto& ebt_item) {
                auto& [ebt, earn] = ebt_item.second;
                result += ebt + earn;
            });

            out_stream_ << result << std::endl;
            return result;
        }

        void Earn(Date from, Date to, double value) {
            size_t size = Date::ComputeDistance(from, to) + 1ul;
            for (int i = 0; i < size; ++i) {
                Date date = from;
                earnings_before_taxes_[date].first += value / size;
                ++from;
            }
        }

        void PayTax(Date from, Date to) {
            auto begin = earnings_before_taxes_.lower_bound(from);
            auto end = earnings_before_taxes_.upper_bound(to);

            std::for_each(begin, end, [](auto& ebt_item) {
                auto& [date, ebt] = ebt_item;
                ebt.second = (ebt.first+ebt.second) * (1.-TAX_RATE);
                ebt.first = 0.;
            });
        }

    private:
        std::ostream& out_stream_;
        std::map<Date, std::pair<double, double>> earnings_before_taxes_;
        //std::map<const Date*, double> tax_payments_;
        //std::map<Date, double> net_incomes_;
    };
}
