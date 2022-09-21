//#include "log_duration.h"
#include <algorithm>
#include <iostream>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>

#include "./../detail/test_framework.h"

using namespace std;

class RandomContainer {
public:
    void Insert(int val) {
        values_pool_.emplace(val);
    }
    void Remove(int val) {
        values_pool_.erase(val);
    }
    bool Has(int val) const {
        return values_pool_.count(val);
    }
    int GetRand() const {
        uniform_int_distribution<int> distr(0, values_pool_.size());
        int rand_index = distr(engine_);
        auto res = values_pool_.lower_bound(rand_index);
        return res == values_pool_.end() ? *std::prev(res) : *res;
    }

private:
    std::set<int> values_pool_;
    mutable mt19937 engine_;
};

int main() {
    RandomContainer container;
    int query_num = 0;
    cin >> query_num;
    {
        // LOG_DURATION("Requests handling"s);

        const auto RequestsHandling = [&container, query_num] {
            for (int query_id = 0; query_id < query_num; query_id++) {
                string query_type;
                cin >> query_type;
                if (query_type == "INSERT"s) {
                    int value = 0;
                    cin >> value;
                    container.Insert(value);
                } else if (query_type == "REMOVE"s) {
                    int value = 0;
                    cin >> value;
                    container.Remove(value);
                } else if (query_type == "HAS"s) {
                    int value = 0;
                    cin >> value;
                    if (container.Has(value)) {
                        cout << "true"s << endl;
                    } else {
                        cout << "false"s << endl;
                    }
                } else if (query_type == "RAND"s) {
                    cout << container.GetRand() << endl;
                }
            }
        };

        transport_catalogue::tests::TestRunner tr;
        RUN_TEST(tr, RequestsHandling);
    }
}