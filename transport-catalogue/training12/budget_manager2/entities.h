#pragma once

struct BudgetSpan {
    double income = 0;
    double spendings = 0;

    friend BudgetSpan operator*(BudgetSpan lhs, double rhs) {
        lhs.income *= rhs;
        lhs.spendings *= rhs;
        return lhs;
    }

    friend BudgetSpan operator+(BudgetSpan lhs, const BudgetSpan rhs) {
        lhs.income += rhs.income;
        lhs.spendings += rhs.spendings;
        return lhs;
    }

    BudgetSpan Tax(double tax) {
        income *= tax;
        return *this;
    }
};