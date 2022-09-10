#include <cassert>
#include <iostream>
#include <string>
#include <string_view>

#include "training12/budget_manager/budget_manager.h"
#include "training12/budget_manager/parser.h"

namespace budget_manager {
    void ParseAndProcessQuery(BudgetManager& manager, std::string&& line) {
        // разработайте функцию чтения и обработки запроса
        auto request = Parser::Parse(std::move(line));
        if (request.cmd == Parser::Command::EARN) {
            assert(request.args.size() == 3);
            manager.Earn(Date::FromString(request.args[0]), Date::FromString(request.args[1]), std::stod(request.args[2]));
        } else if (request.cmd == Parser::Command::PAY_TAX) {
            assert(request.args.size() == 2);
            manager.PayTax(Date::FromString(request.args[0]), Date::FromString(request.args[1]));
        } else if (request.cmd == Parser::Command::COMPUTE_INCOME) {
            assert(request.args.size() == 2);
            manager.ComputeIncome(Date::FromString(request.args[0]), Date::FromString(request.args[1]));
        }
    }

    int ReadNumberOnLine(std::istream& input) {
        std::string line;
        input >> line;
        input.get();
        return std::stoi(line);
    }
}

namespace budget_manager::tests {
    void Test1() {
        std::stringstream ss;
        ss << R"(8
                Earn 2000-01-02 2000-01-06 20
                ComputeIncome 2000-01-01 2001-01-01
                PayTax 2000-01-02 2000-01-03
                ComputeIncome 2000-01-01 2001-01-01
                Earn 2000-01-03 2000-01-03 10
                ComputeIncome 2000-01-01 2001-01-01
                PayTax 2000-01-03 2000-01-03
                ComputeIncome 2000-01-01 2001-01-01)";

        std::ostringstream out;
        BudgetManager manager{out};

        const int query_count = ReadNumberOnLine(ss);

        for (int i = 0; i < query_count; ++i) {
            std::string line;
            std::getline(ss, line);
            ParseAndProcessQuery(manager, std::move(line));
        }

        std::string expected_result = "20\n18.96\n28.96\n27.2076\n";
        if (expected_result != out.str()) {
            std::cerr << "Result: \n";
            std::cerr << out.str() << "\n";
            std::cerr << "Expected: \n";
            std::cerr << expected_result << "\n" << std::endl;
            std::cerr << "Test1: "
                      << "Test FAILED" << std::endl;
            assert(false);
        }

        std::cerr << "Test1: "
                  << "Test DONE" << std::endl;
    }

    void Test2() {
        std::stringstream ss;
        ss << R"(2
                ComputeIncome 2000-01-01 2099-12-31
                Earn 2001-02-12 2001-02-23 921388
                ComputeIncome 2001-02-05 2001-02-16
                PayTax 2001-02-01 2001-02-14 43
                ComputeIncome 2001-02-03 2001-02-13
                PayTax 2001-02-07 2001-02-23 28
                ComputeIncome 2001-02-08 2001-02-11
                PayTax 2001-02-09 2001-02-10 1
                Earn 2001-02-26 2001-02-28 944178
                ComputeIncome 2001-02-12 2001-02-15
                PayTax 2001-02-09 2001-02-27 81
                ComputeIncome 2001-02-12 2001-02-17)";

        std::ostringstream out;
        BudgetManager manager{out};

        const int query_count = ReadNumberOnLine(ss);

        for (int i = 0; i < query_count; ++i) {
            std::string line;
            std::getline(ss, line);
            ParseAndProcessQuery(manager, std::move(line));
        }

        std::string expected_result = "0\n383912\n87531.9\n0\n149818\n49473\n";
        if (expected_result != out.str()) {
            std::cerr << "Result: \n";
            std::cerr << out.str() << "\n";
            std::cerr << "Expected: \n";
            std::cerr << expected_result << "\n" << std::endl;
            std::cerr << "Test1: "
                      << "Test FAILED" << std::endl;
            assert(false);
        }

        std::cerr << "Test2: "
                  << "Test DONE" << std::endl;
    }

    void RunTests() {
        Test1();
        //Test2();

        std::cerr << "All Tests DONE" << std::endl << std::endl;
    }
}

int main() {
    using namespace budget_manager;

    tests::RunTests();

    BudgetManager manager{std::cout};

    const int query_count = ReadNumberOnLine(std::cin);

    for (int i = 0; i < query_count; ++i) {
        std::string line;
        std::getline(std::cin, line);
        ParseAndProcessQuery(manager, std::move(line));
    }
}