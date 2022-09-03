#include "json_builder.h"

#include <optional>

namespace json /* Builder */ {

    using namespace std::literals;

    const Node& Builder::Build() const {
        if (is_empty_ || !nodes_stack_.empty()) {
            throw std::logic_error("Builder state is invalid"s);
        }
        return root_;
    }

    Builder::KeyItemContext Builder::Key(std::string key) {
        if (!nodes_stack_.empty() && nodes_stack_.back()->IsMap() && !has_key_) {
            has_key_ = true;
            key_ = std::move(key);
            return *this;
        }

        throw std::logic_error("Build dictionary error");
    }

    Builder::ValueItemContext Builder::Value(Node value) {
        if (is_empty_) {
            root_ = std::move(value);
            is_empty_ = false;
            return ValueItemContext(*this);
        }

        if (!nodes_stack_.empty() && nodes_stack_.back()->IsMap() && has_key_) {
            nodes_stack_.back()->AsMap().emplace(key_, std::move(value));
            has_key_ = false;
            return *this;
        }

        if (!nodes_stack_.empty() && nodes_stack_.back()->IsArray()) {
            nodes_stack_.back()->AsArray().emplace_back(std::move(value));
            return *this;
        }

        throw std::logic_error("Build value error");
    }

    Builder::DictItemContext Builder::StartDict() {
        Value(Dict{});
        SetRef(Dict{});
        return *this;
    }

    Builder::ItemContext Builder::EndDict() {
        if (!nodes_stack_.empty() && nodes_stack_.back()->IsMap() && !has_key_) {
            nodes_stack_.pop_back();
            return *this;
        }
        throw std::logic_error("Buid dictionary error");
    }

    Builder::ArrayItemContext Builder::StartArray() {
        Value(Array{});
        SetRef(Array{});
        return *this;
    }

    Builder::ItemContext Builder::EndArray() {
        if (!nodes_stack_.empty() && nodes_stack_.back()->IsArray()) {
            nodes_stack_.pop_back();
            return *this;
        }
        throw std::logic_error("Build array error");
    }

    void Builder::SetRef(const Node& value) {
        if (!value.IsArray() && !value.IsMap()) {
            return;
        }
        if (nodes_stack_.empty()) {
            nodes_stack_.push_back(&root_);
        } else if (nodes_stack_.back()->IsArray()) {
            auto* ptr = &nodes_stack_.back()->GetValuePtr<Array>()->back();
            nodes_stack_.push_back(ptr);
        } else if (nodes_stack_.back()->IsMap()) {
            auto ptr = &nodes_stack_.back()->GetValuePtr<Dict>()->at(key_);
            nodes_stack_.push_back(const_cast<Node*>(ptr));
        }
    }

    Builder::ItemContext Builder::GetContext() {
        return *this;
    }
}

namespace json /* Builder::Context */ {

    Builder::KeyValueItemContext Builder::KeyItemContext::Value(Node value) {
        builder_.Value(std::move(value));
        return KeyValueItemContext{builder_};
    }

    Builder::ArrayItemContext Builder::ArrayItemContext::Value(Node value) {
        builder_.Value(std::move(value));
        return ArrayItemContext{builder_};
    }

    Builder::KeyItemContext Builder::ContextBase::Key(std::string key) {
        return builder_.Key(std::move(key));
    }

    Builder::DictItemContext Builder::ContextBase::StartDict() {
        return builder_.StartDict();
    }

    Builder::ItemContext Builder::ContextBase::EndDict() {
        return builder_.EndDict();
    }

    Builder::ArrayItemContext Builder::ContextBase::StartArray() {
        return builder_.StartArray();
    }

    Builder::ItemContext Builder::ContextBase::EndArray() {
        return builder_.EndArray();
    }

    const Node& Builder::ContextBase::Build() const {
        return builder_.Build();
    }

    Builder::ValueItemContext Builder::ItemContext::Value(Node value) {
        return builder_.Value(std::move(value));
    }

}