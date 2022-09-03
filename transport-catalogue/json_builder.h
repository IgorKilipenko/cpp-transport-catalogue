#pragma once

#include <string>
#include <type_traits>
#include <vector>

#include "json.h"

namespace json /* Builder */ {
    class Builder {
        class ContextBase;
        class ItemContext;

        class KeyItemContext;
        class ValueItemContext;
        class KeyValueItemContext;
        class DictItemContext;
        class ArrayItemContext;

    public:
        Builder() = default;

        void Clear() {
            ResetState_(true);
        }

        Node&& Extract() noexcept {
            ResetState_(false);
            return std::move(root_);
        }

        ItemContext GetContext() ;

        ValueItemContext Value(Node value);
        DictItemContext StartDict();
        ArrayItemContext StartArray();

    protected:
        KeyItemContext Key(std::string key);
        ItemContext EndDict();
        ItemContext EndArray();
        const Node& Build() const;

    private:
        void SetRef(const Node& value);

        Node root_;
        std::vector<Node*> nodes_stack_;
        bool has_key_ = false;
        std::string key_;
        bool is_empty_ = true;

    private:
        void ResetState_(bool clear_root = true) {
            is_empty_ = true;
            nodes_stack_.clear();
            has_key_ = false;
            key_.clear();
            if (clear_root) {
                root_ = Node();
            }
        }
    };
}

namespace json /* Builder::Context */ {
    class Builder::ContextBase {
    public:
        ContextBase(Builder& builder) : builder_{builder} {}
        
        operator Builder&() {
            return builder_;
        }

        const Builder& GetBuilder() const {
            return builder_;
        }

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