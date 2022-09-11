#include <algorithm>
#include <cassert>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

using namespace std;

// напишите функцию ComputeStatistics, принимающую 5 параметров:
// два итератора, выходное значение для суммы, суммы квадратов и максимального элемента
template <typename InputIt, typename OutSum, typename OutSqrSum, typename OutMax>
void ComputeStatistics(InputIt first, InputIt last, OutSum& out_sum, OutSqrSum& out_sqr_sum, OutMax& out_max) {
    using Elem = std::decay_t<decltype(*first)>;

    constexpr bool need_sum = !is_same_v<OutSum, const nullopt_t>;
    constexpr bool need_sq_sum = !is_same_v<OutSqrSum, const nullopt_t>;
    constexpr bool need_max = !is_same_v<OutMax, const nullopt_t>;

    if constexpr (need_sum) {
        Elem sum = *first;
        for (auto i = first + 1; i != last; ++i) {
            sum += *i;
        }
        out_sum = sum;
    }
    if constexpr (need_sq_sum) {
        Elem sqr_sum = *first * *first;
        for (auto i = first + 1; i != last; ++i) {
            sqr_sum += *i * *i;
        }
        out_sqr_sum = sqr_sum;
    }
    if constexpr (need_max) {
        Elem max = *first;
        for (auto i = first + 1; i != last; ++i) {
            max = std::max(*i, max);
        }
        out_max = max;
    }
}

struct OnlySum {
    int value;
};

OnlySum operator+(OnlySum l, OnlySum r) {
    return {l.value + r.value};
}
OnlySum& operator+=(OnlySum& l, OnlySum r) {
    return l = l + r;
}

int main() {
    vector input = {1, 2, 3, 4, 5, 6};
    int sq_sum;
    std::optional<int> max;

    // Переданы выходные параметры разных типов - std::nullopt_t, int и std::optional<int>
    ComputeStatistics(input.begin(), input.end(), nullopt, sq_sum, max);

    assert(sq_sum == 91 && max && *max == 6);

    vector<OnlySum> only_sum_vector = {{100}, {-100}, {20}};
    OnlySum sum;

    // Поданы значения поддерживающие только суммирование, но запрошена только сумма
    ComputeStatistics(only_sum_vector.begin(), only_sum_vector.end(), sum, nullopt, nullopt);

    assert(sum.value == 20);
}