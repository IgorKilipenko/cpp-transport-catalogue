#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <functional>
#include <iostream>
#include <istream>
#include <iterator>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace ebooks::detail::string_processing {
    size_t TrimStart(std::string_view& str, const char ch = ' ') {
        size_t idx = str.find_first_not_of(ch);
        if (idx != std::string::npos) {
            str.remove_prefix(idx);
            return idx;
        }
        return 0;
    }

    size_t TrimEnd(std::string_view& str, const char ch = ' ') {
        size_t idx = str.find_last_not_of(ch);
        if (idx != str.npos) {
            str.remove_suffix(str.size() - idx - 1);
            return idx;
        }
        return 0;
    }

    void Trim(std::string_view& str, const char ch = ' ') {
        TrimStart(str, ch);
        TrimEnd(str, ch);
    }
    std::string Trim(std::string&& str, const char ch = ' ') {
        std::string_view tmp{std::move(str)};
        TrimStart(tmp, ch);
        TrimEnd(tmp, ch);
        return std::string(std::move(tmp));
    }

    std::vector<std::string_view> SplitIntoWords(const std::string_view str, const char ch = ' ', size_t max_count = 0) {
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

    template <typename NumType_, typename String_ = std::string&&>
    std::optional<NumType_> TryPaseNumeric(String_&& str, std::function<NumType_(String_)> sto) {
        if (str.empty()) {
            return std::nullopt;
        }

        NumType_ result = 0;
        try {
            result = sto(std::forward<String_>(str));
        } catch (...) {
            return std::nullopt;
        }
        return result;
    }

    template <typename String_ = std::string&&>
    std::optional<int> TryPaseInt(String_&& str) {
        return TryPaseNumeric<int, std::string&&>(std::move(str), [](String_&& str) -> int {
            return std::stoi(std::move(str));
        });
    }
}

namespace ebooks {
    using namespace detail::string_processing;

    struct User {
        int id = 0;

        User() = default;
        explicit User(int id) : id(id) {}
        explicit User(std::string&& str) : id(ParseId_(std::move(str))) {}

    private:
        static int ParseId_(std::string&& str) {
            assert(!str.empty());
            auto id = detail::string_processing::TryPaseInt(std::move(str));
            if (!id.has_value()) {
                throw std::invalid_argument("User id parsing error");
            }
            return id.value();
        }
    };

    struct Hasher {
        uint8_t operator()(const User& user) const {
            return id_hasher_(user.id);
        }
        uint8_t operator()(const User* user) const {
            return ptr_hasher_(user);
        }

    private:
        std::hash<int> id_hasher_{};
        std::hash<const void*> ptr_hasher_{};
    };

    class EbookManager {
    public:
        struct RequestTypes {
            inline static std::string_view READ{"READ"};
            inline static std::string_view CHEER{"CHEER"};
        };

        struct ReadRequest {
            User user;
            size_t page = 0;
        };
        struct CheerRequest {
            User user;
        };

    public:
        EbookManager(std::istream& istream) : in_stream_{istream} {}
        void ProcessRequests() {
            std::string query_count_str = ReadLine_();
            assert(!query_count_str.empty());

            size_t query_count = std::stoull(query_count_str);

            std::vector<std::string> request = ReadLines_(query_count);
            std::for_each(std::make_move_iterator(request.begin()), std::make_move_iterator(request.end()), [](std::string&& req) {
                std::cerr << req << std::endl;
                auto vals_str = detail::string_processing::SplitIntoWords(req);
                assert(!vals_str.empty());
                assert(vals_str.front() == RequestTypes::READ || vals_str.front() == RequestTypes::CHEER);

                std::string_view type = std::move(vals_str.front());
                if (type == RequestTypes::READ) {
                    assert(vals_str.size() == 3);
                    User(static_cast<std::string>(std::move(vals_str[1])));

                } else if (type == RequestTypes::CHEER) {
                    assert(vals_str.size() == 2);
                }

                //! throw std::runtime_error("Invalid request type");
            });
        }

        void ExecuteRequest(ReadRequest&& request) {
            const User user = std::move(request.user);
            users_.emplace(std::move(user));
            /*auto item_it = users_.emplace(static_cast<const User>(std::move(request.user))).first;
            const User* db_user = &(*item_it);
            book_states_[db_user] += request.page;*/
        }
        void ExecuteRequest(CheerRequest&& request) {
            User user = std::move(request.user);
            auto ptr = users_.find(user);
            assert(ptr != users_.end());

            const User* db_user =&(*ptr);
            assert(book_states_.count(db_user));
            size_t page = book_states_[db_user];

            size_t m = 0;
            std::for_each(book_states_.begin(), book_states_.end(), [&m, &db_user, page](const auto& item) {
                if (&item.first == &db_user || item.second >= page) {
                    return;
                }
                ++m;
            });

            double part_res = page == 0ul ? 0. : static_cast<double>(users_.size()) / std::max(1ul, m);
            std::cerr << part_res << std::endl;
        }

    private:
        std::istream& in_stream_;
        std::unordered_set<User, Hasher> users_;
        std::unordered_map<const User*, size_t, Hasher> book_states_;

        std::string ReadLine_() const {
            std::string line;
            std::getline(in_stream_, line);
            return detail::string_processing::Trim(std::move(line));
        }

        std::vector<std::string> ReadLines_(size_t count) const {
            std::vector<std::string> result{count};
            int idx = 0;
            for (std::string line; idx < count && std::getline(in_stream_, line); ++idx) {
                result[idx] = detail::string_processing::Trim(std::move(line));
            }
            return result;
        }
    };
}

int main() {
    using namespace ebooks;

    std::string input =
        R"(12
    CHEER 5
    READ 1 10
    CHEER 1
    READ 2 5
    READ 3 7
    CHEER 2
    CHEER 3
    READ 3 10
    CHEER 3
    READ 3 11
    CHEER 3
    CHEER 1)";

    std::istringstream in{input};

    EbookManager manager{in};
    manager.ProcessRequests();

    return 0;
}
