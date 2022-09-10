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
            // size_t size = Date::ComputeDistance(from, to);
            /*for (int i = 0; i < size; ++i) {
                auto ptr = net_incomes_.find(from);
                result = ptr == net_incomes_.end() ? 0. : result + net_incomes_.at(from);
                ++from;
            }*/

            auto begin = earnings_before_taxes_.lower_bound(from);
            std::cerr << "begin: " << begin->first.ToString() << std::endl;
            auto end = earnings_before_taxes_.upper_bound(to);
            //!end = end == earnings_before_taxes_.end() ? end : std::next(end);
            std::for_each(begin, end, [&result](const auto& ebt_item) {
                auto& [ebt, earn] = ebt_item.second;
                result += ebt + earn * (1. - TAX_RATE);
            });

            out_stream_ << result << std::endl;
            return result;
        }

        void Earn(Date from, Date to, double value) {
            size_t size = Date::ComputeDistance(from, to)+1ul;
            for (int i = 0; i < size; ++i) {
                Date date = from;
                earnings_before_taxes_[date].first += value / size;
                ++from;
            }
        }

        void PayTax(Date from, Date to) {
            auto begin = earnings_before_taxes_.lower_bound(from);
            auto end = earnings_before_taxes_.upper_bound(to);
            //end = end == earnings_before_taxes_.end() ? end : std::next(end);

            std::for_each(begin, end, [this](auto& ebt_item) {
                auto& [date, ebt] = ebt_item;
                double tax_fee = ebt.first * TAX_RATE;
                tax_payments_[&date] += tax_fee;

                ebt.second += ebt.first;
                ebt.first = 0.;
            });
        }

    private:
        std::ostream& out_stream_;
        std::map<Date, std::pair<double, double>> earnings_before_taxes_;
        std::map<const Date*, double> tax_payments_;
        std::map<Date, double> net_incomes_;
    };
}
