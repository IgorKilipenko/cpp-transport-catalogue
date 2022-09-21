#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

struct Person {
    std::string name;
    int age = 0;
};

enum class DBLogLevel;

class DBQuery {
public:
    DBQuery(std::string&& query);
};

struct DBHandler {
    bool IsOK() const;
    std::string Quote(std::string_view) const;

    template <typename Key_, typename Value_>
    std::vector<std::pair<Key_, Value_>> LoadRows(DBQuery) const;
};

struct DbConnectionConfig {
    int db_connection_timeout;
    bool db_allow_exceptions;
    DBLogLevel db_log_level;
};

struct DBConnector {
    bool db_allow_exceptions = false;
    DBLogLevel db_log_level;

    DBConnector(const DbConnectionConfig& config) : db_allow_exceptions(config.db_allow_exceptions), db_log_level(config.db_log_level){};

    DBHandler Connect(std::string_view db_name) {
        return db_name.starts_with("tmp.") ? ConnectTmp_(db_name) : Connect(db_name);
    }

private:
    DBHandler ConnectTmp_(std::string_view db_name);
    DBHandler Connect_(std::string_view db_name);
};

struct Request {
    int min_age = 0;
    int max_age = std::numeric_limits<int>::max();
    std::string_view name_filter;
};

std::vector<Person> LoadPersons(std::string_view db_name, const DbConnectionConfig& db_config, Request&& request) {
    using namespace std::string_literals;
    DBConnector connector(db_config);
    DBHandler db;
    db = connector.Connect(db_name);
    if (!connector.db_allow_exceptions && !db.IsOK()) {
        return {};
    }

    std::ostringstream query_str;
    query_str << "from Persons "s
              << "select Name, Age "s
              << "where Age between "s << request.min_age << " and "s << request.max_age << " "s
              << "and Name like '%"s << db.Quote(request.name_filter) << "%'"s;
    DBQuery query(query_str.str());

    std::vector<Person> persons;
    for (auto [name, age] : db.LoadRows<std::string, int>(query)) {
        persons.emplace_back(move(name), age);
    }
    return persons;
}