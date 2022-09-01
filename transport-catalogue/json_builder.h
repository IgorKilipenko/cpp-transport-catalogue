#pragma once

#include <type_traits>
#include <vector>

#include "json.h"

namespace json /* Builder */ {
    class Builder {
    public:
        template <typename ValueType_>
        class Context {
        public:
            Context(Builder& builder)
                : builder_(builder),
                  value_{(assert(builder_.nodes_stack_.back()->IsType<ValueType_>()), builder_.nodes_stack_.back()->GetValue<ValueType_>())} {}

        protected:
            Builder& builder_;
            ValueType_& value_;
        };

        template <typename ValueType_>
        class NodeContext : public Context<ValueType_> {
            using Context<ValueType_>::Context;

        public:
            template <
                typename Node_,
                json::detail::EnableIf<
                    json::detail::IsConvertibleV<Node_, json::Node> || json::detail::IsConvertibleV<Node_, json::Node::ValueType>> = true>
            NodeContext<ValueType_>& Value(Node_&& value) {
                Context<ValueType_>::value_.emplace(std::move(value));
                return *this;
            }
        };

        template <>
        class NodeContext<Dict> : public Context<Dict> {
            using Context<Dict>::Context;

        public:
            using ValueType = json::Dict;
            using DictContext = NodeContext<Dict>;

            template <typename String_, json::detail::EnableIfConvertible<String_, std::string> = true>
            DictContext Key(String_&& key) {
                // auto it = value_.emplace(std::forward<String_>(key), {}).first;
                // builder_.nodes_stack_.push_back(&it->second);
                auto& node = value_[std::forward<String_>(key)];
                builder_.nodes_stack_.push_back(&node);
                return *this;
            }

            template <
                typename Node_,
                json::detail::EnableIf<
                    json::detail::IsConvertibleV<Node_, json::Node> || json::detail::IsConvertibleV<Node_, json::Node::ValueType>> = true>
            DictContext& Value(Node_&& value) {
                *(builder_.nodes_stack_.back()) = std::forward<Node_>(value);
                return *this;
            }

            DictContext StartDict() {
                return builder_.StartDict();
            }

            DictContext& EndDict() {
                builder_.nodes_stack_.pop_back();
                return *this;
            }

            Node Build() {
                return builder_.Build();
            }
        };
        using DictContext = NodeContext<Dict>::DictContext;

        class ArrayContext : public Context<Array> {
            using Context<Array>::Context;

        public:
            using ValueType = json::Dict;
        };

        DictContext StartDict() {
            if (nodes_stack_.empty()) {
                root_ = json::Dict();
                nodes_stack_.emplace_back(&root_);
            } else if (nodes_stack_.back()->IsArray()) {
                Array& arr = nodes_stack_.back()->GetValue<Array>();
                arr.emplace_back(Dict());
                nodes_stack_.push_back(&arr.back());
            } else if (nodes_stack_.back()->IsNull()) {
                *nodes_stack_.back() = Dict();
            }
            return DictContext(*this);
        }

        template <
            typename Node_, json::detail::EnableIf<
                                json::detail::IsConvertibleV<Node_, json::Node::ValueType> || json::detail::IsConvertibleV<Node_, json::Node>> = true>
        Builder& Value(Node_&& value) {
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

        json::Node Build() {
            return root_;
        }
        Builder& EndDict();
        NodeContext<json::Array> StartArray();
        Builder& EndArray();
        Builder& Key(const std::string& key);

    private:
        json::Node root_;
        std::vector<json::Node*> nodes_stack_;

    private:
        bool IsValidForStartDict_();
        bool IsValidForCloseDict_();
    };
}