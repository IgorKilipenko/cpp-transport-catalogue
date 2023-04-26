#pragma once

#include <contact.pb.h>

#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "iterator_range.h"

struct Date {
    int year, month, day;
};

struct Contact {
    std::string name;
    std::optional<Date> birthday;
    std::vector<std::string> phones;
};

class PhoneBook {
public:
    explicit PhoneBook(std::vector<Contact> contacts);

    // Корректный тип итератора определите сами.
    // Можно и нужно поменять этот using:
    using Iterator = std::vector<Contact>::iterator;
    using ConstIterator = std::vector<Contact>::const_iterator;

    using ContactRange = IteratorRange<ConstIterator>;

    ContactRange FindByNamePrefix(std::string_view name_prefix) const;

    void SaveTo(std::ostream& output) const;

private:
    std::vector<Contact> contacts_;
};

PhoneBook DeserializePhoneBook(std::istream& input);