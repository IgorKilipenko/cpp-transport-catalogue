#include "json_builder.h"

namespace json /* Builder */ {

    Builder::DictContext Builder::StartDict() {
        if (nodes_stack_.empty()) {
            root_ = json::Dict();
            nodes_stack_.emplace_back(&root_);
        } else if (Array* array_ptr = nodes_stack_.back()->GetValuePtr<Array>(); array_ptr != nullptr) {
            array_ptr->emplace_back(Dict());
            nodes_stack_.push_back(&array_ptr->back());
        } else if (nodes_stack_.back()->IsNull()) {
            *nodes_stack_.back() = Dict();
        }
        return DictContext(*this);
    }

    Builder::ArrayContext Builder::StartArray() {
        if (!nodes_stack_.empty() && !nodes_stack_.back()->IsArray() && !nodes_stack_.back()->IsNull()) {
            throw std::logic_error("ArrayContext error");
        }
        if (nodes_stack_.empty()) {
            root_ = Array();
            nodes_stack_.push_back(&root_);
        } else if (Array* array_ptr = nodes_stack_.back()->GetValuePtr<Array>(); array_ptr != nullptr) {
            array_ptr->emplace_back(Array());
            nodes_stack_.push_back(&array_ptr->back());
        } else if (nodes_stack_.back()->IsNull()) {
            *nodes_stack_.back() = Array();
        }
        return ArrayContext(*this);
    }
}

namespace json /* Builder::IObjectContext implementation */ {

    Builder::DictContext Builder::IObjectContext::StartDict() {
        return builder_.StartDict();
    }

    Builder::ArrayContext Builder::IObjectContext::StartArray() {
        return builder_.StartArray();
    }
    // virtual ArrayContext& EndArray();

}