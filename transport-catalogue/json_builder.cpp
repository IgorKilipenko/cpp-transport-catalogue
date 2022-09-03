#include "json_builder.h"

namespace json /* Builder */ {

    using namespace std::literals;

    const Node& Builder::Build() const {
        if (!state_.has_context || !nodes_stack_.empty()) {
            throw std::logic_error("Builder state is invalid"s);
        }
        return root_;
    }

    Builder::DictContext Builder::StartDict() {
        Value(Dict{});
        PutStack<Dict>();
        return *this;
    }

    Builder::Context Builder::EndDict() {
        if (!nodes_stack_.empty() && nodes_stack_.back()->IsMap() && !state_.has_key) {
            nodes_stack_.pop_back();
            return *this;
        }
        throw std::logic_error("Buid dictionary error");
    }

    Builder::ArrayContext Builder::StartArray() {
        Value(Array{});
        PutStack<Array>();
        return *this;
    }

    Builder::Context Builder::EndArray() {
        if (!nodes_stack_.empty() && nodes_stack_.back()->IsArray()) {
            nodes_stack_.pop_back();
            return *this;
        }
        throw std::logic_error("Build array error");
    }

    Builder::Context Builder::GetContext() {
        return *this;
    }

    void Builder::Clear() {
        ResetState_(true);
    }

    Node&& Builder::Extract() noexcept {
        ResetState_(false);
        return std::move(root_);
    }

    void Builder::ResetState_(bool clear_root) {
        state_.Reset();
        nodes_stack_.clear();
        if (clear_root) {
            root_ = Node();
        }
    }
}

namespace json /* Builder::Context implementation */ {

    Builder::DictContext Builder::ContextBase::StartDict() {
        return builder_.StartDict();
    }

    Builder::Context Builder::ContextBase::EndDict() {
        return builder_.EndDict();
    }

    Builder::ArrayContext Builder::ContextBase::StartArray() {
        return builder_.StartArray();
    }

    Builder::Context Builder::ContextBase::EndArray() {
        return builder_.EndArray();
    }

    const Node& Builder::ContextBase::Build() const {
        return builder_.Build();
    }

    Builder::ContextBase::operator Builder&() {
        return builder_;
    }

    const Builder& Builder::ContextBase::GetBuilder() const {
        return builder_;
    }

    Builder::ValueContext Builder::Context::Value(Node value) {
        return builder_.Value(std::move(value));
    }
}
