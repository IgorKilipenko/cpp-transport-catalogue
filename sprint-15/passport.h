#pragma once

#include <ctime>
#include <iostream>
#include <string>

#include "identity_document.h"

using namespace std::string_view_literals;

class Passport {
public:
    Passport() : identity_document_(), expiration_date_(GetExpirationDate()) {
        Passport::SetVtable(this);
        std::cout << "Passport::Ctor()"sv << std::endl;
    }

    Passport(const Passport& other) : identity_document_(other.identity_document_), expiration_date_(other.expiration_date_) {
        Passport::SetVtable(this);
        std::cout << "Passport::CCtor()"sv << std::endl;
    }

    ~Passport() {
        std::cout << "Passport::Dtor()"sv << std::endl;
        IdentityDocument::SetVTable((IdentityDocument*)this);
    }

    void Delete() {
        GetVtable()->delete_this(this);
    }

    void PrintID() const {
        GetVtable()->print_id_this(this);
    }

    void PrintUniqueIDCount() {
        IdentityDocument::PrintUniqueIDCount();
    }

    void PrintVisa(const std::string& country) const {
        GetVtable()->print_visa_this(this, country);
    }

    int GetID() const {
        return identity_document_.GetID();
    }

    Passport& operator=(const Passport&) = delete;

    operator IdentityDocument() {
        return {identity_document_};
    }

    using DeleteFunction = void (*)(Passport*);
    using PrintIDFunction = void (*)(const Passport*);
    using PrintVisaFunction = void (*)(const Passport*, const std::string& country);

    struct Vtable {
        DeleteFunction delete_this;
        PrintIDFunction print_id_this;
        PrintVisaFunction print_visa_this;
    };

    static void SetVtable(Passport* obj) {
        *(Passport::Vtable**)obj = &Passport::VTABLE;
    }

    const Vtable* GetVtable() const {
        return (const Passport::Vtable*)identity_document_.GetVtable();
    }

    Vtable* GetVtable() {
        return (Passport::Vtable*)identity_document_.GetVtable();
    }

    static Passport::Vtable VTABLE;

private:
    IdentityDocument identity_document_;
    const struct tm expiration_date_;

    static void Delete(Passport* obj) {
        delete obj;
    }

    static void PrintID(const Passport* obj) {
        std::cout << "Passport::PrintID() : "sv << obj->GetID();
        std::cout << " expiration date : "sv << obj->expiration_date_.tm_mday << "/"sv << obj->expiration_date_.tm_mon << "/"sv
                  << obj->expiration_date_.tm_year + 1900 << std::endl;
    }

    static void PrintVisa(const Passport* obj, const std::string& country) {
        std::cout << "Passport::PrintVisa("sv << country << ") : "sv << obj->GetID() << std::endl;
    }

    tm GetExpirationDate() {
        time_t t = time(nullptr);
        tm exp_date = *localtime(&t);
        exp_date.tm_year += 10;
        mktime(&exp_date);
        return exp_date;
    }
};

Passport::Vtable Passport::VTABLE = {Passport::Delete, Passport::PrintID, Passport::PrintVisa};
