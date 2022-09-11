#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

class RouteManager {
public:
    void AddRoute(int start, int finish) {
        reachable_lists_[start].emplace(finish);
        reachable_lists_[finish].emplace(start);
    }
    int FindNearestFinish(int start, int finish) const {
        int result = abs(start - finish);
        auto ptr = reachable_lists_.find(start);
        if (ptr == reachable_lists_.end()) {
            return result;
        }
        const auto& reachable_stations = ptr->second;
        if (!reachable_stations.empty()) {
            auto it = reachable_stations.lower_bound(finish);
            if (it == reachable_stations.end()) {
                result = std::min(result, std::abs(*(--it) - finish));
            } else if (it == reachable_stations.begin()) {
                result = std::min(result, std::abs(*it - finish));
            } else {
                result = std::min({result, std::abs(*it - finish), std::abs(*(--it) - finish)});
            }
        }

        return result;
    }

private:
    std::unordered_map<int, std::set<int>> reachable_lists_;
};

int main() {
    RouteManager routes;

    int query_count;
    cin >> query_count;

    for (int query_id = 0; query_id < query_count; ++query_id) {
        string query_type;
        cin >> query_type;
        int start, finish;
        cin >> start >> finish;
        if (query_type == "ADD"s) {
            routes.AddRoute(start, finish);
        } else if (query_type == "GO"s) {
            cout << routes.FindNearestFinish(start, finish) << "\n"s;
        }
    }

    cout.flush();
}