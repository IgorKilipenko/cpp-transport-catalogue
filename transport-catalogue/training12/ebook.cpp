#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <iostream>
#include <istream>
#include <iterator>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

namespace ebooks::detail::string_processing /* detail */ {
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
    std::optional<NumType_> TryParseNumeric(String_&& str, std::function<NumType_(String_)> sto) {
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
    std::optional<int> TryParseInt(String_&& str) {
        return TryParseNumeric<int, std::string&&>(std::move(str), [](String_&& str) -> int {
            return std::stoi(std::move(str));
        });
    }
}

namespace ebooks /* User */ {
    using namespace detail::string_processing;

    struct User {
        int id = 0;

        User() = default;
        explicit User(int id) : id(id) {}
        explicit User(std::string&& str) : id(ParseId(std::move(str))) {}

        static int ParseId(std::string&& str) {
            assert(!str.empty());
            auto id = TryParseInt(std::move(str));
            if (!id.has_value()) {
                throw std::invalid_argument("User id parsing error");
            }
            return id.value();
        }

        struct ByIdEqual {
        public:
            bool operator()(const User& lhs, const User& rhs) const {
                return lhs.id == rhs.id;
            }
        };

        struct Hasher {
            uint8_t operator()(const User& user) const {
                return id_hasher_(user.id);
            }

        private:
            std::hash<int> id_hasher_{};
        };
    };
}

namespace ebooks /* EbookManager */ {

    class EbookManager {
    public:
        struct RequestTypes;
        struct ReadRequest;
        struct CheerRequest;
        using UserTable = std::unordered_map<User, size_t, User::Hasher, User::ByIdEqual>;

    public:
        EbookManager(std::istream& istream, std::ostream& ostream) : in_stream_{istream}, out_stream_{ostream} {}
        void ProcessRequests();
        void ExecuteRequest(ReadRequest&& request);
        void ExecuteRequest(CheerRequest&& request);

    private:
        std::istream& in_stream_;
        std::ostream& out_stream_;
        UserTable users_table_;
        std::map<size_t, size_t> reading_progress_;

    private: /* private methods */
        std::string ReadLine_() const;
        std::vector<std::string> ReadLines_(size_t count) const;
    };
}

namespace ebooks /* EbookManager::Requests */ {

    struct EbookManager::RequestTypes {
        inline static std::string_view READ{"READ"};
        inline static std::string_view CHEER{"CHEER"};
    };

    struct EbookManager::ReadRequest {
        User user;
        size_t page = 0;

        ReadRequest() = default;
        ReadRequest(User&& user, size_t page) : user{std::move(user)}, page{page} {}
        ReadRequest(std::string&& user_str, std::string&& page_str) : user(User(std::move(user_str))), page(ParsePageNumber(std::move(page_str))) {}

        static size_t ParsePageNumber(std::string&& str) {
            auto page = TryParseNumeric<size_t, std::string&&>(std::move(str), [](std::string&& str) {
                return std::stoul(std::move(str));
            });
            if (!page.has_value()) {
                throw std::invalid_argument("Page number parsing error");
            }
            return page.value();
        }
    };

    struct EbookManager::CheerRequest {
        User user;

        CheerRequest() = default;
        explicit CheerRequest(User&& user) : user{std::move(user)} {}
        explicit CheerRequest(std::string&& str) : user(User(std::move(str))) {}
    };
}

namespace ebooks /* EbookManager implementation */ {
    using namespace detail::string_processing;

    void EbookManager::ProcessRequests() {
        auto query_count = TryParseNumeric<size_t, std::string&&>(ReadLine_(), [](std::string&& str) {
            return std::stoul(std::move(str));
        });
        if (!query_count.has_value()) {
            throw std::runtime_error("Request processing error. Invalid request size value");
        }

        std::vector<std::string> request = ReadLines_(query_count.value());
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

            } else if (type == RequestTypes::CHEER) {
                assert(vals_str.size() == 2);
                User user(static_cast<std::string>(std::move(vals_str[1])));

                ExecuteRequest(CheerRequest(std::move(user)));

            } else {
                throw std::runtime_error("Invalid request type");
            }
        });

        out_stream_.flush();
    }

    void EbookManager::ExecuteRequest(ReadRequest&& request) {
        size_t new_page = std::move(request.page);
        auto [it, success] = users_table_.emplace(std::move(request.user), new_page);
        auto* stat = &reading_progress_[it->second];
        if (!success) {
            --*stat;
            it->second = new_page;
            stat = &reading_progress_[it->second];
        }
        ++*stat;
    }

    void EbookManager::ExecuteRequest(CheerRequest&& request) {
        const auto send = [this](double value) {
            out_stream_ << value << out_stream_.widen('\n');
        };

        double result = 0.;
        auto user_ptr = users_table_.find(request.user);
        if (user_ptr == users_table_.end()) {
            send(result);
            return;
        }

        size_t page = user_ptr->second;
        if (users_table_.size() == 1) {
            result = 1.;
            send(result);
            return;
        }

        auto stat_first_it = reading_progress_.find(page);
        assert(stat_first_it != reading_progress_.end());
        assert(stat_first_it->second);

        size_t top_readers_count = 0;
        std::for_each(stat_first_it, reading_progress_.end(), [&top_readers_count](const auto& stat_item) {
            top_readers_count += stat_item.second;
        });
        size_t wrost_users = users_table_.size() - top_readers_count;

        result = wrost_users / static_cast<double>(users_table_.size() - 1ul);
        send(result);
    }

    std::string EbookManager::ReadLine_() const {
        std::string line;
        std::getline(in_stream_, line);
        return detail::string_processing::Trim(std::move(line));
    }

    std::vector<std::string> EbookManager::ReadLines_(size_t count) const {
        std::vector<std::string> result{count};
        size_t idx = 0;
        for (std::string line; idx < count && std::getline(in_stream_, line); ++idx) {
            result[idx] = detail::string_processing::Trim(std::move(line));
        }
        return result;
    }
}

int main() {
    using namespace ebooks;

    EbookManager manager{std::cin, std::cout};
    manager.ProcessRequests();

    return 0;
}