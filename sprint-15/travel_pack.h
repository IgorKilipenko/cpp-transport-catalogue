#pragma once

#include <iostream>
#include <string>

#include "driving_licence.h"
#include "identity_document.h"
#include "passport.h"

using namespace std::string_view_literals;

class TravelPack {
public:
    TravelPack()
        : identity_document_base_(), identity_doc1_((IdentityDocument*)(new Passport())), identity_doc2_((IdentityDocument*)(new DrivingLicence())) {
        TravelPack::SetVtable(this);
        std::cout << "TravelPack::Ctor()"sv << std::endl;
    }

    TravelPack(const TravelPack& other)
        : identity_document_base_(other.identity_document_base_),
          identity_doc1_((IdentityDocument*)(new Passport(*(Passport*)(other.identity_doc1_)))),
          identity_doc2_((IdentityDocument*)(new DrivingLicence(*(DrivingLicence*)(other.identity_doc2_)))),
          additional_pass_(other.additional_pass_),
          additional_dr_licence_(other.additional_dr_licence_) {
        TravelPack::SetVtable(this);
        std::cout << "TravelPack::CCtor()"sv << std::endl;
    }

    ~TravelPack() {
        identity_doc1_->Delete();
        identity_doc2_->Delete();
        std::cout << "TravelPack::Dtor()"sv << std::endl;
        IdentityDocument::SetVTable((IdentityDocument*)this);
    }

    void Delete() {
        GetVtale()->delete_this(this);
    }

    void PrintID() const {
        GetVtale()->print_id(this);
    }

    void PrintUniqueIDCount() {
        IdentityDocument::PrintUniqueIDCount();
    }

    int GetID() const {
        return identity_document_base_.GetID();
    }

    operator IdentityDocument() {
        return {identity_document_base_};
    }

    using DeleteFunction = void (*)(TravelPack*);
    using PrintIDFunction = void (*)(const TravelPack*);

    struct Vtable {
        DeleteFunction delete_this;
        PrintIDFunction print_id;
    };

    static void SetVtable(TravelPack* obj) {
        *(TravelPack::Vtable**)obj = &TravelPack::VTABLE;
    }

    const Vtable* GetVtale() const {
        return (const TravelPack::Vtable*)identity_document_base_.GetVtable();
    }

    Vtable* GetVtale() {
        return (TravelPack::Vtable*)identity_document_base_.GetVtable();
    }

    static TravelPack::Vtable VTABLE;

private:
    IdentityDocument identity_document_base_;
    IdentityDocument* identity_doc1_;
    IdentityDocument* identity_doc2_;
    Passport additional_pass_;
    DrivingLicence additional_dr_licence_;

    static void Delete(TravelPack* obj) {
        delete obj;
    }

    static void PrintID(const TravelPack* obj) {
        obj->identity_doc1_->PrintID();
        obj->identity_doc2_->PrintID();
        obj->additional_pass_.PrintID();
        obj->additional_dr_licence_.PrintID();
    }
};

TravelPack::Vtable TravelPack::VTABLE = {TravelPack::Delete, TravelPack::PrintID};
