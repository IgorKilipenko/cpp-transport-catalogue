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
        class Context;
        class KeyContext;
        class ValueContext;
        class KeyValueContext;
        class DictContext;
        class ArrayContext;

    private:
        struct ContextState {
            bool has_key = false;
            std::string key;
            bool has_context = false;
            Node* current_value_ptr = nullptr;

            void Reset() {
                has_key = false;
                key.clear();
                has_context = false;
                current_value_ptr = nullptr;
            }
        };

    public:
        Builder() = default;

        template <
            typename NodeType_,
            detail::EnableIf<detail::IsConvertibleV<NodeType_, Node> || detail::IsConvertibleV<NodeType_, Node::ValueType>> = true>
        ValueContext Value(NodeType_&& value);

        DictContext StartDict();
        ArrayContext StartArray();

        void Clear();
        Node&& Extract() noexcept;
        Context GetContext();

    protected:
        template <typename KeyType_, detail::EnableIf<detail::IsConvertibleV<KeyType_, std::string>> = true>
        KeyContext Key(KeyType_&& key);
        Context EndDict();
        Context EndArray();
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
        template <typename KeyType_, detail::EnableIf<detail::IsConvertibleV<KeyType_, std::string>> = true>
        KeyContext Key(KeyType_&& key);
        DictContext StartDict();
        Context EndDict();
        ArrayContext StartArray();
        Context EndArray();
        const Node& Build() const;

    protected:
        Builder& builder_;
    };

    class Builder::Context : public ContextBase {
        using ContextBase::ContextBase;

    public: /* Base class methods */
        using ContextBase::Build;
        using ContextBase::EndArray;
        using ContextBase::EndDict;
        using ContextBase::Key;
        using ContextBase::StartArray;
        using ContextBase::StartDict;

    public:
        ValueContext Value(Node value);
    };

    class Builder::KeyValueContext : public ContextBase {
        using ContextBase::ContextBase;

    public:
        using ContextBase::EndDict;
        using ContextBase::Key;
    };

    class Builder::ValueContext : public ContextBase {
        using ContextBase::ContextBase;

    public:
        using ContextBase::Build;
        using ContextBase::EndArray;
        using ContextBase::EndDict;
        using ContextBase::Key;
        using ContextBase::StartArray;
        using ContextBase::StartDict;
    };

    class Builder::KeyContext : public ContextBase {
        using ContextBase::ContextBase;

    public:
        using ContextBase::StartArray;
        using ContextBase::StartDict;

    public:
        template <
            typename NodeType_,
            detail::EnableIf<detail::IsConvertibleV<NodeType_, Node> || detail::IsConvertibleV<NodeType_, Node::ValueType>> = true>
        KeyValueContext Value(NodeType_&& value);
    };

    class Builder::DictContext : public ContextBase {
        using ContextBase::ContextBase;

    public:
        using ContextBase::EndDict;
        using ContextBase::Key;
    };

    class Builder::ArrayContext : public ContextBase {
        using ContextBase::ContextBase;

    public:
        using ContextBase::EndArray;
        using ContextBase::StartArray;
        using ContextBase::StartDict;

    public:
        template <
            typename NodeType_,
            detail::EnableIf<detail::IsConvertibleV<NodeType_, Node> || detail::IsConvertibleV<NodeType_, Node::ValueType>> = true>
        ArrayContext Value(NodeType_&& value);
    };
}

namespace json /* Builder template implementation */ {

    template <typename NodeType_, detail::EnableIf<detail::IsConvertibleV<NodeType_, Node> || detail::IsConvertibleV<NodeType_, Node::ValueType>>>
    Builder::ValueContext Builder::Value(NodeType_&& value) {
        if (!state_.has_context) {
            root_ = std::forward<NodeType_>(value);
            state_.has_context = true;
            return *this;
        }
        if (auto* map_ptr = !nodes_stack_.empty() ? nodes_stack_.back()->GetValuePtr<Dict>() : nullptr; map_ptr != nullptr) {
            state_.current_value_ptr = &(map_ptr->emplace(state_.key, std::forward<NodeType_>(value)).first->second);
            state_.has_key = false;
            return *this;
        }
        if (auto* array_ptr = !nodes_stack_.empty() ? nodes_stack_.back()->GetValuePtr<Array>() : nullptr; array_ptr != nullptr) {
            array_ptr->emplace_back(std::forward<NodeType_>(value));
            return *this;
        }

        throw std::logic_error("Build value error");
    }

    template <typename KeyType_, detail::EnableIf<detail::IsConvertibleV<KeyType_, std::string>>>
    Builder::KeyContext Builder::Key(KeyType_&& key) {
        if (!nodes_stack_.empty() && nodes_stack_.back()->IsMap() && !state_.has_key) {
            state_.has_key = true;
            state_.key = std::forward<KeyType_>(key);
            return *this;
        }

        throw std::logic_error("Build dictionary error");
    }

    template <typename NodeType_, detail::EnableIf<detail::IsSameV<NodeType_, Dict> || detail::IsSameV<NodeType_, Array>>>
    void Builder::PutStack() {
        if (nodes_stack_.empty()) {
            nodes_stack_.emplace_back(&root_);
        } else if (auto* array_ptr = nodes_stack_.back()->GetValuePtr<Array>(); array_ptr != nullptr) {
            nodes_stack_.emplace_back(&array_ptr->back());
        } else if ((assert(state_.current_value_ptr != nullptr), nodes_stack_.back()->IsMap())) {
            nodes_stack_.emplace_back(state_.current_value_ptr);
        }
    }

}

namespace json /* Builder::ContextBase template implementation */ {

    template <typename KeyType_, detail::EnableIf<detail::IsConvertibleV<KeyType_, std::string>>>
    Builder::KeyContext Builder::ContextBase::Key(KeyType_&& key) {
        return builder_.Key(std::forward<KeyType_>(key));
    }

    template <typename NodeType_, detail::EnableIf<detail::IsConvertibleV<NodeType_, Node> || detail::IsConvertibleV<NodeType_, Node::ValueType>>>
    Builder::KeyValueContext Builder::KeyContext::Value(NodeType_&& value) {
        builder_.Value(std::forward<NodeType_>(value));
        return builder_;
    }

    template <typename NodeType_, detail::EnableIf<detail::IsConvertibleV<NodeType_, Node> || detail::IsConvertibleV<NodeType_, Node::ValueType>>>
    Builder::ArrayContext Builder::ArrayContext::Value(NodeType_&& value) {
        builder_.Value(std::forward<NodeType_>(value));
        return builder_;
    }
}