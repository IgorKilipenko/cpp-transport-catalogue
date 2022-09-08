#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

using namespace std;

class Domain {
    // разработайте класс домена

    // конструктор должен позволять конструирование из string, с сигнатурой определитесь сами

    // разработайте operator==

    // разработайте метод IsSubdomain, принимающий другой домен и возвращающий true, если this его поддомен

public:
    template <typename String_, std::enable_if_t<std::is_convertible_v<std::decay_t<String_>, std::string>, bool> = true>
    Domain(String_&& str) : domain_{(assert(!std::string(str).empty()), std::forward<String_>(str))}, parts_(Split_(domain_)) {
        assert(!parts_.empty());
        std::reverse(parts_.begin(), parts_.end());
    }

    Domain(const Domain& other) : domain_(other.domain_) {
        if (this == &other) {
            return;
        }
        CopyParts_(other);
    }

    Domain& operator=(const Domain& other) {
        if (this == &other) {
            return *this;
        }
        domain_ = other.domain_;
        parts_.clear();
        CopyParts_(other);
        return *this;
    }

    bool operator==(const Domain& other) const {
        return this == &other || domain_ == other.domain_;
    }

    bool operator!=(const Domain& other) const {
        return !(*this == other);
    }

    bool IsSubdomain(const Domain& domain) const {
        if (domain.parts_.size() >= parts_.size()) {
            return false;
        }
        for (auto it = domain.parts_.begin(), sub_it = parts_.begin(); it != domain.parts_.end(); ++it, ++sub_it) {
            if (*it != *sub_it) {
                return false;
            }
        }
        return true;
    }

public:
    struct Compare {
    public:
        bool operator()(const Domain& lhs, const Domain& rhs) const {
            return comparer_(lhs.parts_, rhs.parts_);

            // return std::lexicographical_compare(
            //     lhs.parts_.begin(), lhs.parts_.end(), rhs.parts_.begin(), rhs.parts_.end(),
            //     [](const std::string_view& lhs, const std::string_view& rhs) {
            //         return lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
            //     });
        }

    private:
        std::less<std::vector<std::string_view>> comparer_;
    };

private:
    std::string domain_;
    std::vector<std::string_view> parts_;

    static std::vector<std::string_view> Split_(std::string_view str, const char ch = '.', size_t max_count = 0) {
        if (str.empty()) {
            return {};
        }
        std::vector<std::string_view> result;

        do {
            int64_t pos = str.find(ch, 0);
            std::string_view substr = (pos == static_cast<int64_t>(str.npos)) ? str.substr(0) : str.substr(0, pos);
            result.push_back(std::move(substr));
            str.remove_prefix(std::min(str.find_first_not_of(ch, pos), str.size()));
            if (max_count && result.size() == max_count - 1) {
                result.push_back(std::move(str));
                break;
            }
        } while (!str.empty());

        return result;
    }

    void CopyParts_(const Domain& other) {
        assert(other.domain_ == domain_);

        parts_.reserve(other.parts_.size());
        for (const auto& p : other.parts_) {
            const size_t first = p.data() - other.domain_.data();
            const size_t size = p.size();
            parts_.push_back(std::string_view(domain_.data() + first, size));
        }
    }
};

class DomainChecker {
public:
    // конструктор должен принимать список запрещённых доменов через пару итераторов

    // разработайте метод IsForbidden, возвращающий true, если домен запрещён

    template <
        typename IteratorType_,
        std::enable_if_t<std::is_convertible_v<typename std::iterator_traits<IteratorType_>::value_type, Domain>, bool> = true>
    DomainChecker(IteratorType_ first, IteratorType_ last) : forbidden_domains_{first, last} {
        std::sort(forbidden_domains_.begin(), forbidden_domains_.end(), Domain::Compare());
        auto end = std::unique(forbidden_domains_.begin(), forbidden_domains_.end(), [](const Domain& lhs, const Domain& rhs) {
            return lhs == rhs || lhs.IsSubdomain(rhs) || rhs.IsSubdomain(lhs);
        });
        forbidden_domains_.erase(end, forbidden_domains_.end());
    }

    const std::vector<Domain>& GetForbiddenDomains() const {
        return forbidden_domains_;
    }

    bool IsForbidden(const Domain& domain) const {
        auto it = std::upper_bound(forbidden_domains_.begin(), forbidden_domains_.end(), domain, Domain::Compare());
        return it == forbidden_domains_.begin() ? false : *std::prev(it) == domain || domain.IsSubdomain(*std::prev(it));
    }

private:
    std::vector<Domain> forbidden_domains_;
};

// разработайте функцию ReadDomains, читающую заданное количество доменов из стандартного входа

template <typename Number>
Number ReadNumberOnLine(istream& input) {
    string line;
    getline(input, line);

    Number num;
    std::istringstream(line) >> num;

    return num;
}

std::vector<Domain> ReadDomains(std::istream& in, const size_t domains_count) {
    std::vector<Domain> domains;
    domains.reserve(domains_count);
    std::string domain_name;

    for (size_t i = 0; i < domains_count; ++i) {
        in >> domain_name;
        std::string reverse(domain_name.rbegin(), domain_name.rend());
        domains.push_back({ reverse + '.'});
    }    
    return domains;
}

void TestDomain() {
    using namespace std::string_literals;
    // Tset IsSubdomain
    {
        Domain domain("mail.com");
        Domain subdomain("test.mail.com");
        assert(subdomain.IsSubdomain(domain));
        assert(!domain.IsSubdomain(subdomain));
        assert(!domain.IsSubdomain(domain));
        assert(Domain("test.1.mail.empty.test.mail.com"s).IsSubdomain(domain));
        assert(Domain("test.1.mail.empty.test.mail.com"s).IsSubdomain(subdomain));
        assert(Domain("test.1.mail.empty.test.mail.com"s).IsSubdomain(Domain("com"s)));
        assert(Domain("foo.any.google.com"s).IsSubdomain(Domain("any.google.com"s)));
        assert(!Domain("test.1.mail.empty.test.mail.com"s).IsSubdomain(Domain("mail.ru"s)));
    }
    // Tsets Equal
    {
        assert(Domain("mail.com"s) != Domain("test.mail.com"s));
        assert(Domain("mail.com"s) == Domain("mail.com"s));
        // assert(Domain("mail.com"s) == Domain(".mail.com"s));
    }

    std::cerr << "Domain: "
              << "Test DONE" << std::endl;
}

void TestDomainChecker() {
    // Test sort and make unique forbidden domains
    {
        std::vector<Domain> forbidden_domains{
            "mail.com_2"s, "test.mail.com_3"s, "test.1.mail.empty.test.mail.com_1"s, "1.mail.empty.test.mail.com_1"s};
        DomainChecker checker(forbidden_domains.begin(), forbidden_domains.end());

        std::vector<Domain> expected_result{forbidden_domains[3], forbidden_domains[0], forbidden_domains[1]};

        assert(checker.GetForbiddenDomains() == expected_result);
    }
    {
        std::vector<Domain> forbidden_domains{"mail.com"s, "test.mail.com"s, "test.1.mail.empty.test.mail.com"s, "1.mail.empty.test.mail.com"s,
                                              "ru",        "any.google.com"s};
        DomainChecker checker(forbidden_domains.begin(), forbidden_domains.end());

        std::vector<Domain> domains{"google.com", "yandex.ru", "mail.ru", "mail.com", "foo.any.google.com"};

        for (auto it = domains.begin(); it != domains.end();) {
            if (checker.IsForbidden(*it)) {
                it = domains.erase(it);
                continue;
            }
            ++it;
        }
        assert(!domains.empty());
        std::vector<Domain> expected_result{domains[0]};

        assert(domains == expected_result);
    }

    std::cerr << "DomainChecker: "
              << "Test DONE" << std::endl;
}

int main() {
    TestDomain();
    TestDomainChecker();
    const std::vector<Domain> forbidden_domains = ReadDomains(cin, ReadNumberOnLine<size_t>(cin));
    DomainChecker checker(forbidden_domains.begin(), forbidden_domains.end());

    const std::vector<Domain> test_domains = ReadDomains(cin, ReadNumberOnLine<size_t>(cin));
    for (const Domain& domain : test_domains) {
        cout << (checker.IsForbidden(domain) ? "Bad"sv : "Good"sv) << endl;
    }
    return 0;
}