#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <istream>
#include <iterator>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

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
        explicit User(std::string&& str) : id(ParseId(std::move(str))) {}

        static int ParseId(std::string&& str) {
            assert(!str.empty());
            auto id = TryPaseInt(std::move(str));
            if (!id.has_value()) {
                throw std::invalid_argument("User id parsing error");
            }
            return id.value();
        }

        struct ByIdCompare {
        public:
            bool operator()(const User& lhs, const User& rhs) const {
                return id_equal_(lhs.id, rhs.id);
            }

        private:
            std::unordered_set<int>::key_equal id_equal_;
        };
    };

    struct Hasher {
        template <typename UserType_, std::enable_if_t<std::is_same_v<std::decay_t<UserType_>, User>, bool> = true>
        uint8_t operator()(UserType_&& user) const {
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

            ReadRequest() = default;
            ReadRequest(User&& user, size_t page) : user{std::move(user)}, page{page} {}
            ReadRequest(std::string&& user_str, std::string&& page_str)
                : user(User(std::move(user_str))), page(ParsePageNumber(std::move(page_str))) {}

            static size_t ParsePageNumber(std::string&& str) {
                auto page = TryPaseNumeric<size_t, std::string&&>(std::move(str), [](std::string&& str) {
                    return std::stoul(std::move(str));
                });
                if (!page.has_value()) {
                    throw std::invalid_argument("Page number parsing error");
                }
                return page.value();
            }
        };
        struct CheerRequest {
            User user;

            CheerRequest() = default;
            explicit CheerRequest(User&& user) : user{std::move(user)} {}
            explicit CheerRequest(std::string&& str) : user(User(std::move(str))) {}
        };

    public:
        EbookManager(std::istream& istream, std::ostream& ostream) : in_stream_{istream}, out_stream_{ostream} {}
        void ProcessRequests() {
            std::string query_count_str = ReadLine_();
            assert(!query_count_str.empty());

            size_t query_count = std::stoull(query_count_str);

            std::vector<std::string> request = ReadLines_(query_count);
            std::for_each(std::make_move_iterator(request.begin()), std::make_move_iterator(request.end()), [this](std::string&& req) {
                auto vals_str = detail::string_processing::SplitIntoWords(req);
                assert(!vals_str.empty());
                assert(vals_str.front() == RequestTypes::READ || vals_str.front() == RequestTypes::CHEER);

                std::string_view type = std::move(vals_str.front());
                if (type == RequestTypes::READ) {
                    assert(vals_str.size() == 3);
                    User user(static_cast<std::string>(std::move(vals_str[1])));
                    size_t page = ReadRequest::ParsePageNumber(static_cast<std::string>(std::move(vals_str.back())));
                    ExecuteRequest(ReadRequest(std::move(user), page));
                    return;

                } else if (type == RequestTypes::CHEER) {
                    assert(vals_str.size() == 2);
                    User user(static_cast<std::string>(std::move(vals_str[1])));
                    ExecuteRequest(CheerRequest(std::move(user)));
                    return;
                }

                throw std::runtime_error("Invalid request type");
            });
        }

        void ExecuteRequest(ReadRequest&& request) {
            size_t new_page = std::move(request.page);
            auto [it, success] = users_read_table_.emplace(std::move(request.user), new_page);
            auto& stat = pages_read_stat_[it->second];
            if (!success) {
                stat.extract(&it->first);
                it->second = new_page;
                stat = pages_read_stat_[it->second];
            }
            stat.emplace(&it->first);
        }

        void ExecuteRequest(CheerRequest&& request) {
            auto user_ptr = users_read_table_.find(request.user);
            if (user_ptr == users_read_table_.end()) {
                out_stream_ << 0. << std::endl;
                return;
            }

            size_t page = user_ptr->second;

            if (page == 0) {
                out_stream_ << 0. << std::endl;
                return;
            }

            if (users_read_table_.size() == 1) {
                out_stream_ << 1. << std::endl;
                return;
            }

            size_t wrost_users_count = 0;
            std::for_each(
                std::next(users_read_table_.begin()), users_read_table_.end(), [&wrost_users_count, page, user_ptr](const auto& other_user) {
                    if (user_ptr->first.id == other_user.first.id || other_user.second >= page) {
                        return;
                    }
                    ++wrost_users_count;
                });

            double part_res = wrost_users_count / static_cast<double>(std::max(users_read_table_.size() - 1ul, 1ul));
            out_stream_ << part_res << std::endl;
        }

    private:
        std::istream& in_stream_;
        std::ostream& out_stream_;
        std::unordered_map<User, size_t, Hasher, User::ByIdCompare> users_read_table_;
        std::map<size_t, std::unordered_set<const User*, Hasher>> pages_read_stat_;

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

    EbookManager manager{in, std::cout};
    manager.ProcessRequests();

    return 0;
}
