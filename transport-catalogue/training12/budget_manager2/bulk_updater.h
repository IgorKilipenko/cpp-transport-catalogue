#pragma once

#include <cmath>
#include <cstdint>
#include <functional>
#include <numeric>
#include <utility>
#include <vector>

#include "entities.h"
#include "summing_segment_tree.h"

struct BulkMoneyAdder {
    BudgetSpan delta = {};
};

struct BulkTaxApplier {
    BulkTaxApplier() {}

    BulkTaxApplier(int tax_rate) {
        factors.push_back(1 - (0.01 * tax_rate));
    }

    double ComputeFactor() const {
        return std::accumulate(factors.begin(), factors.end(), 1.0, std::multiplies<double>());
    }

    std::vector<double> factors;
};

class BulkLinearUpdater {
public:
    BulkLinearUpdater() = default;

    BulkLinearUpdater(const BulkMoneyAdder& add) : add_(add) {}

    BulkLinearUpdater(const BulkTaxApplier& tax) : tax_(tax) {}

    void CombineWith(const BulkLinearUpdater& other) {
        tax_.factors.reserve(tax_.factors.size() + other.tax_.factors.size());
        tax_.factors.insert(tax_.factors.end(), other.tax_.factors.begin(), other.tax_.factors.end());
        add_.delta = add_.delta.Tax(other.tax_.ComputeFactor()) + other.add_.delta;
    }

    BudgetSpan Collapse(BudgetSpan origin, IndexSegment segment) const {
        return origin.Tax(tax_.ComputeFactor()) + add_.delta * static_cast<double>(segment.length());
    }

private:
    BulkTaxApplier tax_;
    BulkMoneyAdder add_;
};