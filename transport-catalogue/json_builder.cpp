#include "json_builder.h"

#include <optional>

namespace json /* Builder */ {

    using namespace std::literals;

    const Node& Builder::Build() const {
        if (!state_.has_context || !nodes_stack_.empty()) {
            throw std::logic_error("Builder state is invalid"s);
        }
        return root_;
    }

    Builder::DictItemContext Builder::StartDict() {
        Value(Dict{});
        PutStack<Dict>();
        return *this;
    }

    Builder::ItemContext Builder::EndDict() {
        if (!nodes_stack_.empty() && nodes_stack_.back()->IsMap() && !state_.has_key) {
            nodes_stack_.pop_back();
            return *this;
        }
        throw std::logic_error("Buid dictionary error");
    }

    Builder::ArrayItemContext Builder::StartArray() {
        Value(Array{});
        PutStack<Array>();
        return *this;
    }

    Builder::ItemContext Builder::EndArray() {
        if (!nodes_stack_.empty() && nodes_stack_.back()->IsArray()) {
            nodes_stack_.pop_back();
            return *this;
        }
        throw std::logic_error("Build array error");
    }

    Builder::ItemContext Builder::GetContext() {
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

namespace json /* Builder::ContextBase implementation */ {

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

    Builder::ContextBase::operator Builder&() {
        return builder_;
    }

    const Builder& Builder::ContextBase::GetBuilder() const {
        return builder_;
    }
}

namespace json /* Builder::ItemsContext */ {

    Builder::KeyValueItemContext Builder::KeyItemContext::Value(Node value) {
        builder_.Value(std::move(value));
        return KeyValueItemContext{builder_};
    }

    Builder::ArrayItemContext Builder::ArrayItemContext::Value(Node value) {
        builder_.Value(std::move(value));
        return ArrayItemContext{builder_};
    }

    Builder::ValueItemContext Builder::ItemContext::Value(Node value) {
        return builder_.Value(std::move(value));
    }
}
