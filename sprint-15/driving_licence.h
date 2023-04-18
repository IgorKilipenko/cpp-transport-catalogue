#pragma once

#include <iostream>
#include <string>

#include "identity_document.h"

using namespace std::string_view_literals;

class DrivingLicence {
public:
    static void SetVTable(DrivingLicence* obj) {
        *(DrivingLicence::Vtable**)obj = &DrivingLicence::VTABLE;
    }

    DrivingLicence() : identity_doc_() {
        DrivingLicence::SetVTable(this);
        std::cout << "DrivingLicence::Ctor()"sv << std::endl;
    }

    DrivingLicence(const DrivingLicence& other) : identity_doc_(other.identity_doc_) {
        DrivingLicence::SetVTable(this);
        std::cout << "DrivingLicence::CCtor()"sv << std::endl;
    }

    ~DrivingLicence() {
        std::cout << "DrivingLicence::Dtor()"sv << std::endl;
        IdentityDocument::SetVTable((IdentityDocument*)this);
    }

    void Delete() {
        GetVtable()->delete_this(this);
    }

    void PrintID() const {
        GetVtable()->print_id(this);
    }

    void PrintUniqueIDCount() {
        IdentityDocument::PrintUniqueIDCount();
    }

    int GetID() const {
        return identity_doc_.GetID();
    }

    operator IdentityDocument() {
        return {identity_doc_};
    }

    using DeleteFunction = void (*)(DrivingLicence*);
    using PrintIDFunction = void (*)(const DrivingLicence*);

    struct Vtable {
        DeleteFunction delete_this;
        PrintIDFunction print_id;
    };

    const Vtable* GetVtable() const {
        return (const DrivingLicence::Vtable*)identity_doc_.GetVtable();
    }

    Vtable* GetVtable() {
        return (DrivingLicence::Vtable*)identity_doc_.GetVtable();
    }

    static DrivingLicence::Vtable VTABLE;

private:
    IdentityDocument identity_doc_;
    static void Delete(DrivingLicence* obj) {
        delete obj;
    }

    static void PrintID(const DrivingLicence* obj) {
        std::cout << "DrivingLicence::PrintID() : "sv << obj->GetID() << std::endl;
    }
};

DrivingLicence::Vtable DrivingLicence::VTABLE = {DrivingLicence::Delete, DrivingLicence::PrintID};
