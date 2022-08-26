#pragma once

#include "../detail/type_traits.h"
#include "../json.h"

namespace transport_catalogue::tests {

    template <typename Result, detail::EnableIf<detail::IsConvertibleV<Result, json::Document> || detail::IsBaseOfV<json::Node, Result>> = true>
    void CheckResults(Result &&expected_result, Result &&result) {
        // if (result != expected_result) {
        if constexpr (detail::IsBaseOfV<json::Node, Result>) {
            if (result.EqualsWithTolerance(expected_result)) {
                return;
            }
        } else {
            if (result == expected_result) {
                return;
            }
        }

        std::cerr << "Test result:" << std::endl;
        result.Print(std::cerr);
        std::cerr << std::endl;

        std::cerr << std::endl << "Test expected result:" << std::endl;
        expected_result.Print(std::cerr);

        assert(false);
    }

    inline void CheckResultsExtend(json::Array &&expected_result, json::Array &&result) {
        assert(expected_result.size() == result.size());
        for (int i = 0; i < expected_result.size(); ++i) {
            CheckResults(expected_result[i], result[i]);
        }
    }

    template <typename String, detail::EnableIfConvertible<String, std::string_view> = true>
    void CheckResults(String &&expected_result, String &&result) {
        if (result != expected_result) {
            std::cerr << "Test result:" << std::endl;
            std::cerr << result << std::endl;

            std::cerr << std::endl << "Test expected result:" << std::endl;
            std::cerr << expected_result << std::endl;

            assert(false);
        }
    }
}