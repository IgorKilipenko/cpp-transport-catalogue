#pragma once

#include "../detail/type_traits.h"
#include "../json.h"

namespace transport_catalogue::tests {

    inline size_t TrimStart(std::string_view &str, const char ch = ' ') {
        size_t idx = str.find_first_not_of(ch);
        if (idx != std::string::npos) {
            str.remove_prefix(idx);
            return idx;
        }
        return 0;
    }

    inline size_t TrimEnd(std::string_view &str, const char ch = ' ') {
        size_t idx = str.find_last_not_of(ch);
        if (idx != str.npos) {
            str.remove_suffix(str.size() - idx - 1);
            return idx;
        }
        return 0;
    }

    inline void Trim(std::string_view &str, const char ch = ' ') {
        TrimStart(str, ch);
        TrimEnd(str, ch);
    }

    inline std::vector<std::string_view> SplitIntoWords(const std::string_view str, const char ch = ' ', size_t max_count = 0) {
        if (str.empty()) {
            return {};
        }
        std::string_view str_cpy = str;
        std::vector<std::string_view> result;
        Trim(str_cpy);

        do {
            int64_t pos = str_cpy.find(ch, 0);
            std::string_view substr = (pos == static_cast<int64_t>(str_cpy.npos)) ? str_cpy.substr(0) : str_cpy.substr(0, pos);
            Trim(substr);
            result.push_back(std::move(substr));
            str_cpy.remove_prefix(std::min(str_cpy.find_first_not_of(ch, pos), str_cpy.size()));
            if (max_count && result.size() == max_count - 1) {
                Trim(str_cpy);
                result.push_back(std::move(str_cpy));
                break;
            }
        } while (!str_cpy.empty());

        return result;
    }

    template <typename Result, detail::EnableIf<detail::IsConvertibleV<Result, json::Document> || detail::IsBaseOfV<json::Node, Result>> = true>
    void CheckResults(Result &&expected_result, Result &&result) {
        if (result == expected_result) {
            return;
        }

        std::cerr << "Test result:" << std::endl;
        result.Print(std::cerr);
        std::cerr << std::endl;

        std::cerr << std::endl << "Test expected result:" << std::endl;
        expected_result.Print(std::cerr);

        assert(false);
    }

    inline void CheckResultsExtend(json::Array &&expected_result, json::Array &&result, double tolerance = json::Node::GetEqualityTolerance()) {
        assert(expected_result.size() == result.size());

        double prev_tolerance = json::Node::GetEqualityTolerance();
        json::Node::SetEqualityTolerance(tolerance);

        for (size_t i = 0; i < expected_result.size(); ++i) {
            CheckResults(expected_result[i], result[i]);
        }

        json::Node::SetEqualityTolerance(prev_tolerance);
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