#pragma once

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "json.h"

namespace json /* Builder */ {
    class Builder {
        class ContextBase;

    public:
        class ItemContext;
        class KeyItemContext;
        class ValueItemContext;
        class KeyValueItemContext;
        class DictItemContext;
        class ArrayItemContext;

    private:
        struct ContextState {
            bool has_key = false;
            std::string key;
            bool has_context = false;

            void Reset() {
                has_key = false;
                key.clear();
                has_context = false;
            }
        };

    public:
        Builder() = default;

        template <
            typename NodeType_,
            detail::EnableIf<detail::IsConvertibleV<NodeType_, Node> || detail::IsConvertibleV<NodeType_, Node::ValueType>> = true>
        ValueItemContext Value(NodeType_&& value);

        DictItemContext StartDict();
        ArrayItemContext StartArray();

        void Clear();
        Node&& Extract() noexcept;
        ItemContext GetContext();

    protected:
        KeyItemContext Key(std::string key);
        ItemContext EndDict();
        ItemContext EndArray();
        const Node& Build() const;

    private:
        Node root_;
        std::vector<Node*> nodes_stack_;
        ContextState state_;

    private:
        template <typename NodeType_, detail::EnableIf<detail::IsSameV<NodeType_, Dict> || detail::IsSameV<NodeType_, Array>> = true>
        void PutStack();
        void ResetState_(bool clear_root = true);
    };
}

namespace json /* Builder::Context */ {
    class Builder::ContextBase {
    public:
        ContextBase(Builder& builder) : builder_{builder} {}

        operator Builder&();
        const Builder& GetBuilder() const;

    protected:
        KeyItemContext Key(std::string key);
        DictItemContext StartDict();
        ItemContext EndDict();
        ArrayItemContext StartArray();
        ItemContext EndArray();
        const Node& Build() const;

    protected:
        Builder& builder_;
    };

    class Builder::ItemContext : public ContextBase {
        using ContextBase::ContextBase;

    public: /* Base class methods */
        using ContextBase::Build;
        using ContextBase::EndArray;
        using ContextBase::EndDict;
        using ContextBase::Key;
        using ContextBase::StartArray;
        using ContextBase::StartDict;

    public:
        ValueItemContext Value(Node value);
    };

    class Builder::KeyValueItemContext : public ContextBase {
        using ContextBase::ContextBase;

    public:
        using ContextBase::EndDict;
        using ContextBase::Key;
    };

    class Builder::ValueItemContext : public ContextBase {
        using ContextBase::ContextBase;

    public:
        using ContextBase::Build;
        using ContextBase::EndArray;
        using ContextBase::EndDict;
        using ContextBase::Key;
        using ContextBase::StartArray;
        using ContextBase::StartDict;
    };

    class Builder::KeyItemContext : public ContextBase {
        using ContextBase::ContextBase;

    public:
        using ContextBase::StartArray;
        using ContextBase::StartDict;

    public:
        KeyValueItemContext Value(Node value);
    };

    class Builder::DictItemContext : public ContextBase {
        using ContextBase::ContextBase;

    public:
        using ContextBase::EndDict;
        using ContextBase::Key;
    };

    class Builder::ArrayItemContext : public ContextBase {
        using ContextBase::ContextBase;

    public:
        using ContextBase::EndArray;
        using ContextBase::StartArray;
        using ContextBase::StartDict;

    public:
        ArrayItemContext Value(Node value);
    };
}

namespace json /* Builder template implementation */ {

    template <typename NodeType_, detail::EnableIf<detail::IsConvertibleV<NodeType_, Node> || detail::IsConvertibleV<NodeType_, Node::ValueType>>>
    Builder::ValueItemContext Builder::Value(NodeType_&& value) {
        if (!state_.has_context) {
            root_ = std::forward<NodeType_>(value);
            state_.has_context = true;
            return *this;
        }

        if (!nodes_stack_.empty() && nodes_stack_.back()->IsMap() && state_.has_key) {
            nodes_stack_.back()->AsMap().emplace(state_.key, std::forward<NodeType_>(value));
            state_.has_key = false;
            return *this;
        }

        if (!nodes_stack_.empty() && nodes_stack_.back()->IsArray()) {
            nodes_stack_.back()->AsArray().emplace_back(std::forward<NodeType_>(value));
            return *this;
        }

        throw std::logic_error("Build value error");
    }

    template <typename NodeType_, detail::EnableIf<detail::IsSameV<NodeType_, Dict> || detail::IsSameV<NodeType_, Array>>>
    void Builder::PutStack() {
        if (nodes_stack_.empty()) {
            nodes_stack_.emplace_back(&root_);
        } else if (auto* ptr = nodes_stack_.back()->GetValuePtr<Array>(); ptr != nullptr) {
            nodes_stack_.emplace_back(&ptr->back());
        } else if (auto* ptr = nodes_stack_.back()->GetValuePtr<Dict>(); ptr != nullptr) {
            nodes_stack_.emplace_back(&ptr->at(state_.key));
        }
    }

}