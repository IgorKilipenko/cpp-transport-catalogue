#pragma once

#include <type_traits>
#include <vector>

#include "json.h"

namespace json /* Builder */ {
    class Builder {
    public:
        template <typename ValueType_, json::detail::EnableIfConvertible<ValueType_, Node::ValueType> = true>
        class Context;

        template <typename ValueType_>
        class NodeContext;

        //template <>
        //class NodeContext<Dict>;
        using DictContext = NodeContext<Dict>;

        //template <>
        //class NodeContext<Array>;
        using ArrayContext = NodeContext<Array>;

        class IObjectContext;

    public:
        DictContext StartDict();

        ArrayContext StartArray();

        template <
            typename Node_, json::detail::EnableIf<
                                json::detail::IsConvertibleV<Node_, json::Node::ValueType> || json::detail::IsConvertibleV<Node_, json::Node>> = true>
        Builder& Value(Node_&& value);

        json::Node Build() {
            return root_;
        }

    private:
        json::Node root_;
        std::vector<json::Node*> nodes_stack_;

    private:
        bool IsValidForStartDict_();
        bool IsValidForCloseDict_();
    };
}

namespace json /* Builder::Context */ {
    template <typename ValueType_, json::detail::EnableIfConvertible<ValueType_, Node::ValueType>>
    class Builder::Context {
    public:
        // Context(Builder& builder)
        //     : builder_(builder),
        //       value_{(assert(builder_.nodes_stack_.back()->IsType<ValueType_>()), builder_.nodes_stack_.back()->GetValue<ValueType_>())} {}
        Context(ValueType_& value) : value_{value} {}

    protected:
        // Builder& builder_;
        ValueType_& value_;
    };

    template <typename ValueType_>
    class Builder::NodeContext : public Builder::Context<ValueType_> {};
}

namespace json /* IBuilder */ {
    class Builder::IObjectContext {
    public:
        IObjectContext(Builder& builder) : builder_(builder) {}
        virtual ~IObjectContext() = default;
        virtual DictContext StartDict();
        virtual DictContext& EndDict() {
            throw "Error";
        }
        virtual ArrayContext StartArray();
        virtual ArrayContext& EndArray() {
            throw "Error";
        }
        virtual Node Build() {
            return builder_.Build();
        }

    protected:
        Builder& builder_;
    };
}

namespace json /* Builder::DictContext */ {
    template <>
    class Builder::NodeContext<Dict> : public Builder::Context<Dict>, public Builder::IObjectContext {
        // using Context<Dict>::Context;

    public:
        using ValueType = json::Dict;
        using DictContext = NodeContext<Dict>;

        NodeContext<ValueType>(Builder& builder)
            : Context<ValueType>((assert(builder.nodes_stack_.back()->IsType<ValueType>()), builder.nodes_stack_.back()->GetValue<ValueType>())),
              IObjectContext(builder) {}

        template <typename String_, json::detail::EnableIfConvertible<String_, std::string> = true>
        DictContext Key(String_&& key) {
            // auto it = value_.emplace(std::forward<String_>(key), {}).first;
            // builder_.nodes_stack_.push_back(&it->second);
            auto& node = value_[std::forward<String_>(key)];
            builder_.nodes_stack_.push_back(&node);
            return *this;
        }

        template <
            typename Node_, json::detail::EnableIf<
                                json::detail::IsConvertibleV<Node_, json::Node> || json::detail::IsConvertibleV<Node_, json::Node::ValueType>> = true>
        DictContext& Value(Node_&& value) {
            *(builder_.nodes_stack_.back()) = std::forward<Node_>(value);
            return *this;
        }

        /*DictContext StartDict() override {
            return builder_.StartDict();
        }*/

        DictContext& EndDict() override {
            builder_.nodes_stack_.pop_back();
            return *this;
        }

        /*Node Build() {
            return builder_.Build();
        }*/

    private:
        using IObjectContext::EndArray;
    };
}

namespace json /* Builder::ArrayContext */ {

    template <>
    class Builder::NodeContext<Array> : public Context<Array>, public Builder::IObjectContext {
        // using Context<Array>::Context;

    public:
        using ValueType = json::Array;
        using ArrayContext = NodeContext<Array>;

        NodeContext<ValueType>(Builder& builder)
            : Context<ValueType>((assert(builder.nodes_stack_.back()->IsType<ValueType>()), builder.nodes_stack_.back()->GetValue<ValueType>())),
              IObjectContext(builder) {}

        ArrayContext& EndArray() override {
            builder_.nodes_stack_.pop_back();
            return *this;
        }

        /*Node Build() {
            return builder_.Build();
        }*/

        template <
            typename Node_, json::detail::EnableIf<
                                json::detail::IsConvertibleV<Node_, json::Node> || json::detail::IsConvertibleV<Node_, json::Node::ValueType>> = true>
        ArrayContext& Value(Node_&& value) {
            builder_.nodes_stack_.back()->AsArray().emplace_back(std::forward<Node_>(value));
            return *this;
        }

    private:
        using IObjectContext::EndDict;
    };

}

namespace json /* Builder template implementation */ {
    template <
        typename Node_,
        json::detail::EnableIf<json::detail::IsConvertibleV<Node_, json::Node::ValueType> || json::detail::IsConvertibleV<Node_, json::Node>>>
    Builder& Builder::Value(Node_&& value) {
        // using T = std::decay_t<decltype(value)>;
        if (nodes_stack_.empty()) {
            root_ = Node(value);
            nodes_stack_.push_back(&root_);
        }
        if (!nodes_stack_.back()->IsArray() && !nodes_stack_.back()->IsNull()) {
            throw std::logic_error("Value error");
        } else if (nodes_stack_.back()->IsArray()) {
            Array& arr = nodes_stack_.back()->GetValue<Array>();
            arr.emplace_back(std::forward<Node_>(value));
        }
        return *this;
    }
}