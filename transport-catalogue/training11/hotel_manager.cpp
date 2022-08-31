#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <set>

using namespace std;

class HotelManager {
    std::queue<int64_t> time_;
    // 1. time   2. name     3. id   4. rooms
    std::map<int64_t, std::map<std::pair<std::string, int>, int>> registration_;

public:
    void Book(const int64_t time, const std::string &hotel_name, const int client_id, const int rooms_count) {
        time_.push(time);
        registration_[time][{hotel_name, client_id}] += rooms_count;

        while (time - 86400 >= time_.front()) {
            time_.pop();
        }
    };

    int ComputeClientCount(const std::string &hotel_name) {
        std::set<int64_t> ids;
        for (const auto &[time, data] : registration_) {
            if (time <= time_.back() && time >= time_.front()) {
                for (const auto &[p, rooms] : data) {
                    if (p.first == hotel_name) {
                        ids.emplace(p.second);
                    }
                }
            }
        }
        return ids.size();
    }
    int ComputeRoomCount(const std::string &hotel_name) {
        int result = 0;
        for (const auto &[time, data] : registration_) {
            if (time <= time_.back() && time >= time_.front()) {
                for (const auto &[p, rooms] : data) {
                    if (p.first == hotel_name) {
                        result += rooms;
                    }
                }
            }
        }
        return result;
    }
};

int main() {
    HotelManager manager;

    int query_count;
    cin >> query_count;

    for (int query_id = 0; query_id < query_count; ++query_id) {
        string query_type;
        cin >> query_type;

        if (query_type == "BOOK") {
            int64_t time;
            cin >> time;
            string hotel_name;
            cin >> hotel_name;
            int client_id, room_count;
            cin >> client_id >> room_count;
            manager.Book(time, hotel_name, client_id, room_count);
        } else {
            string hotel_name;
            cin >> hotel_name;
            if (query_type == "CLIENTS") {
                cout << manager.ComputeClientCount(hotel_name) << "\n";
            } else if (query_type == "ROOMS") {
                cout << manager.ComputeRoomCount(hotel_name) << "\n";
            }
        }
    }

    return 0;
}