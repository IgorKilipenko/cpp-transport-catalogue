#include "phone_book.h"
#include <utility>

#include "contact.pb.h"

using namespace std;

// Напишите тут реализации методов и функций,
// декларированных в phone_book.h
PhoneBook DeserializePhoneBook(std::istream& input) {
    phone_book_serialize::ContactList object;
    if (!object.ParseFromIstream(&input)) {
        return PhoneBook({});
    }

    std::vector<Contact> temporary_contacts;
    for (const auto& contact : object.contact()) {
        Contact temporary_contact{contact.name()};
        if (contact.has_birthday()) {
            temporary_contact.birthday = Date(
                {static_cast<int>(contact.birthday().year()), static_cast<int>(contact.birthday().month()),
                 static_cast<int>(contact.birthday().day())});
        }
        if (!contact.phone_number().empty()) {
            std::for_each(contact.phone_number().begin(), contact.phone_number().end(), [&temporary_contact](auto&& phone) {
                temporary_contact.phones.push_back(std::forward<typeof phone>(phone));
            });
        }
        temporary_contacts.push_back(std::move(temporary_contact));
    }

    return PhoneBook(temporary_contacts);
}

PhoneBook::PhoneBook(std::vector<Contact> contacts) : contacts_(contacts) {
    std::sort(contacts_.begin(), contacts_.end(), [](auto&& lhs, auto&& rhs) {
        return std::lexicographical_compare(lhs.name.begin(), lhs.name.end(), rhs.name.begin(), rhs.name.end());
    });
}

PhoneBook::ContactRange PhoneBook::FindByNamePrefix(std::string_view name_prefix) const {
    if (name_prefix.empty()) {
        return ContactRange{contacts_.begin(), contacts_.end()};
    }

    auto it_begin = std::lower_bound(contacts_.begin(), contacts_.end(), name_prefix, [](const Contact& lhs, const std::string_view name_prefix) {
        return lhs.name < name_prefix;
    });

    auto it_end = std::upper_bound(contacts_.begin(), contacts_.end(), name_prefix, [](const std::string_view name_prefix, const Contact& rhs) {
        return name_prefix < rhs.name.substr(0, std::min(rhs.name.length(), name_prefix.length()));
    });

    ContactRange range{it_begin, it_end};

    return range;
}

void PhoneBook::SaveTo(std::ostream& output) const {
    phone_book_serialize::ContactList object;
    phone_book_serialize::Contact temporary_contact;

    std::for_each(contacts_.begin(), contacts_.end(), [&temporary_contact, &object](Contact contact) {
        temporary_contact.set_name(contact.name);

        if (contact.birthday.has_value()) {
            phone_book_serialize::Date date;
            date.set_year(contact.birthday->year);
            date.set_month(contact.birthday->month);
            date.set_day(contact.birthday->day);
            *temporary_contact.mutable_birthday() = std::move(date);
        }

        if (!contact.phones.empty()) {
            std::for_each(contact.phones.begin(), contact.phones.end(), [&temporary_contact](std::string phone) {
                temporary_contact.add_phone_number(std::move(phone));
            });
        }

        object.mutable_contact()->Add(std::move(temporary_contact));
        temporary_contact.clear_birthday();
        temporary_contact.clear_name();
        temporary_contact.clear_phone_number();
    });
    object.SerializeToOstream(&output);
}