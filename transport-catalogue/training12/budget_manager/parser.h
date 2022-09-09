#pragma once
#include <algorithm>
#include <cassert>
#include <iterator>

#include "date.h"

// напишите в этом классе код, ответственный за чтение запросов
#pragma once

namespace budget_manager {
    class Parser {
    public:
        enum class Command : uint8_t {
            INVALID,
            COMPUTE_INCOME,
            EARN,
            PAY_TAX,
        };

        struct CommandValue {
            inline static std::string INVALID = "";
            inline static std::string COMPUTE_INCOME = "ComputeIncome";
            inline static std::string EARN = "Earn";
            inline static std::string PAY_TAX = "PayTax";
        };

        struct Request {
            Command cmd = Command::INVALID;
            std::vector<std::string> args;
        };

    public:
        static Request Parse(std::string&& request) {
            std::vector<std::string_view> parts = Split_(request);
            assert(!parts.empty());

            std::string cmd_str = std::string(std::move(parts.front()));
            Command cmd = Command::INVALID;
            if (cmd_str == CommandValue::COMPUTE_INCOME) {
                cmd = Command::COMPUTE_INCOME;
            } else if (cmd_str == CommandValue::EARN) {
                cmd = Command::EARN;
            } else if (cmd_str == CommandValue::PAY_TAX) {
                cmd = Command::PAY_TAX;
            }

            std::vector<std::string> args(parts.size() - 1);
            std::transform(
                std::make_move_iterator(parts.begin() + 1), std::make_move_iterator(parts.end()), args.begin(), [](std::string_view&& str) {
                    std::string res(std::move(str));
                    return res;
                });

            return {cmd, std::move(args)};
        };

        static std::vector<std::string_view> Split_(std::string_view str, const char ch = ' ') {
            if (str.empty()) {
                return {};
            }

            std::vector<std::string_view> result;
            str.remove_prefix(std::min(str.find_first_not_of(ch, 0), str.size()));

            while (!str.empty()) {
                int64_t pos = str.find(ch, 0);
                std::string_view substr = (pos == static_cast<int64_t>(str.npos)) ? str.substr(0) : str.substr(0, pos);
                result.push_back(std::move(substr));
                str.remove_prefix(std::min(str.find_first_not_of(ch, pos), str.size()));
            }

            return result;
        }
    };
}