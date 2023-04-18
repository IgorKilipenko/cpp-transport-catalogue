#pragma once

#include <iostream>
#include <string>

#include "driving_licence.h"

using namespace std::string_view_literals;

class InternationalDrivingLicence {
public:
    InternationalDrivingLicence() : driving_licence_() {
        InternationalDrivingLicence::SetVtable(this);
        std::cout << "InternationalDrivingLicence::Ctor()"sv << std::endl;
    }

    InternationalDrivingLicence(const InternationalDrivingLicence& other) : driving_licence_(other.driving_licence_) {
        InternationalDrivingLicence::SetVtable(this);
        std::cout << "InternationalDrivingLicence::CCtor()"sv << std::endl;
    }

    ~InternationalDrivingLicence() {
        std::cout << "InternationalDrivingLicence::Dtor()"sv << std::endl;
        DrivingLicence::SetVTable((DrivingLicence*)this);
    }

    InternationalDrivingLicence& operator=(const InternationalDrivingLicence&) = delete;

    using DeleteFunction = void (*)(InternationalDrivingLicence*);
    using PrintIDFunction = void (*)(const InternationalDrivingLicence*);

    struct Vtable {
        DeleteFunction delete_this;
        PrintIDFunction print_id_this;
    };

    void Delete() {
        GetVtable()->delete_this(this);
    }

    void PrintID() const {
        GetVtable()->print_id_this(this);
    }

    int GetID() const {
        return driving_licence_.GetID();
    }

    operator DrivingLicence() {
        return {driving_licence_};
    }

    static void SetVtable(InternationalDrivingLicence* obj) {
        *(InternationalDrivingLicence::Vtable**)obj = &InternationalDrivingLicence::VTABLE;
    }

    const Vtable* GetVtable() const {
        return (const InternationalDrivingLicence::Vtable*)driving_licence_.GetVtable();
    }

    Vtable* GetVtable() {
        return (InternationalDrivingLicence::Vtable*)driving_licence_.GetVtable();
    }

    static InternationalDrivingLicence::Vtable VTABLE;

private:
    DrivingLicence driving_licence_;

    static void Delete(InternationalDrivingLicence* obj) {
        delete obj;
    }

    static void PrintID(const InternationalDrivingLicence* obj) {
        std::cout << "InternationalDrivingLicence::PrintID() : "sv << obj->GetID() << std::endl;
    }
};

InternationalDrivingLicence::Vtable InternationalDrivingLicence::VTABLE = {InternationalDrivingLicence::Delete, InternationalDrivingLicence::PrintID};
