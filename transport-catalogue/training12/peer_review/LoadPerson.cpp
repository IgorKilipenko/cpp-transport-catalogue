#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

struct Person {
    std::string name;
    int age;
};
class DBLogLevel {};
class DBQuery {
public:
    DBQuery(std::string&& query) : query_(std::move(query)) {}

private:
    std::string query_;
};

struct DBHandler {
    bool IsOK() const {
        return false;
    }
    std::string Quote(std::string_view) const {
        return "";
    }

    template <typename Key_, typename Value_>
    std::vector<std::pair<Key_, Value_>> LoadRows(DBQuery) const {
        return {};
    }
};

struct DBConnector {
    bool db_allow_exceptions = false;
    DBLogLevel db_log_level;

    DBConnector(bool db_allow_exceptions, DBLogLevel db_log_level) : db_allow_exceptions(db_allow_exceptions), db_log_level(db_log_level){};

    DBHandler ConnectTmp(std::string_view db_name, int db_connection_timeout) {
        return {};
    }
    DBHandler Connect(std::string_view db_name, int db_connection_timeout) {
        return {};
    }
};

std::vector<Person> LoadPersons(
    std::string_view db_name, int db_connection_timeout, bool db_allow_exceptions, DBLogLevel db_log_level, int min_age, int max_age,
    std::string_view name_filter) {
    using namespace std::string_literals;
    DBConnector connector(db_allow_exceptions, db_log_level);
    DBHandler db;
    if (db_name.starts_with("tmp."s)) {
        db = connector.ConnectTmp(db_name, db_connection_timeout);
    } else {
        db = connector.Connect(db_name, db_connection_timeout);
    }
    if (!db_allow_exceptions && !db.IsOK()) {
        return {};
    }

    std::ostringstream query_str;
    query_str << "from Persons "s
              << "select Name, Age "s
              << "where Age between "s << min_age << " and "s << max_age << " "s
              << "and Name like '%"s << db.Quote(name_filter) << "%'"s;
    DBQuery query(query_str.str());

    std::vector<Person> persons;
    for (auto [name, age] : db.LoadRows<std::string, int>(query)) {
        persons.push_back({move(name), age});
    }
    return persons;
}