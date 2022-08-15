#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <iostream>
#include <istream>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace json::detail {
    template <bool Condition>
    using EnableIf = typename std::enable_if_t<Condition, bool>;

    template <typename FromType, typename ToType>
    using EnableIfConvertible = std::enable_if_t<std::is_convertible_v<std::decay_t<FromType>, ToType>, bool>;

    template <typename FromType, typename ToType>
    using IsConvertible = std::is_convertible<std::decay_t<FromType>, ToType>;

    template <typename FromType, typename ToType>
    inline constexpr bool IsConvertibleV = std::is_convertible<std::decay_t<FromType>, ToType>::value;

    template <typename FromType, typename ToType>
    using IsSame = std::is_same<std::decay_t<FromType>, ToType>;

    template <typename FromType, typename ToType>
    inline constexpr bool IsSameV = std::is_same<std::decay_t<FromType>, ToType>::value;

    template <typename FromType, typename ToType>
    using EnableIfSame = std::enable_if_t<std::is_same_v<std::decay_t<FromType>, ToType>, bool>;

    template <typename BaseType, typename DerivedType>
    using IsBaseOf = std::is_base_of<BaseType, std::decay_t<DerivedType>>;

    template <typename FromType, typename ToType>
    inline constexpr bool IsBaseOfV = IsBaseOf<FromType, ToType>::value;

    template <typename BaseType, typename DerivedType>
    using EnableIfBaseOf = std::enable_if_t<std::is_base_of_v<BaseType, std::decay_t<DerivedType>>, bool>;
}

namespace json {
    class Node;
    using Array = std::vector<Node>;
    using DictBase = std::map<std::string, Node, std::less<>>;

    using Numeric = std::variant<int, double>;

    class Dict : public DictBase {
        using DictBase::map;

    public:
        using ValueType = DictBase::mapped_type;
        using KeyType = DictBase::key_type;

        template <
            typename KeyType = Dict::KeyType,
            detail::EnableIf<
                detail::IsConvertibleV<KeyType, Dict::KeyType> ||
                (detail::IsConvertibleV<KeyType, std::string_view> && detail::IsConvertibleV<KeyType, std::string_view>)> = true>
        const ValueType* Find(KeyType&& key) const {
            const auto ptr = find(std::forward<KeyType>(key));
            return ptr == end() ? nullptr : &ptr->second;
        }

        template <
            typename KeyType = Dict::KeyType,
            detail::EnableIf<
                detail::IsConvertibleV<KeyType, Dict::KeyType> ||
                (detail::IsConvertibleV<KeyType, std::string_view> && detail::IsConvertibleV<KeyType, std::string_view>)> = true>
        ValueType* Find(KeyType&& key) {
            const auto ptr = find(std::forward<KeyType>(key));
            return ptr == end() ? nullptr : &ptr->second;
        }
    };

    using NodeValueType = std::variant<std::nullptr_t, std::string, int, double, bool, json::Array, json::Dict>;

    class Node : private NodeValueType {
    public:
        // Делаем доступными все конструкторы родительского класса variant
        using variant::variant;
        using ValueType = variant;

        bool IsNull() const;

        bool IsBool() const;

        bool IsInt() const;

        bool IsDouble() const;

        bool IsPureDouble() const;

        bool IsString() const;

        bool IsArray() const;

        bool IsMap() const;

        template <typename T, detail::EnableIfConvertible<T, NodeValueType> = true>
        bool IsType() const;

        template <typename T = void, detail::EnableIf<detail::IsConvertible<T, NodeValueType>::value || detail::IsSame<T, void>::value> = true>
        const auto& GetValue() const;

        template <typename T = void, detail::EnableIf<detail::IsConvertible<T, NodeValueType>::value || detail::IsSame<T, void>::value> = true>
        auto&& ExtractValue() {
            /*if (!std::holds_alternative<T>(*this)) {
                throw std::logic_error("Node don't contained this type alternative");
            }
            return std::get<T>(std::move(*this));*/
            if constexpr (detail::IsConvertible<T, NodeValueType>::value == true) {
                if (!std::holds_alternative<T>(*this)) {
                    throw std::logic_error("Node don't contained this type alternative");
                }
                return std::get<T>(std::move(*this));
            } else {
                return static_cast<Node::ValueType&&>(std::move(*this));
            }
        }

        template <typename T = void, detail::EnableIf<detail::IsConvertible<T, NodeValueType>::value || detail::IsSame<T, void>::value> = true>
        const auto* GetValuePtr() const noexcept;

        const Node::ValueType* GetValuePtr() const;

        bool AsBool() const;

        int AsInt() const;

        double AsDouble() const;

        const std::string& AsString() const;

        std::string&& ExtractString() {
            // return std::get<std::string>(std::move(*this));
            return ExtractValue<std::string>();
        }

        const json::Array& AsArray() const;

        json::Array&& ExtractArray() {
            // return std::get<json::Array>(std::move(*this));
            return ExtractValue<json::Array>();
        }

        const json::Dict& AsMap() const;

        json::Dict&& ExtractMap() {
            /*ValueType tmp;
            ValueType::swap(tmp);
            return std::get<json::Dict>(std::move(tmp));*/

            // return std::get<json::Dict>(std::move(*this));
            return ExtractValue<json::Dict>();
        }

        bool operator==(const Node& rhs) const;

        bool operator!=(const Node& rhs) const;

        static Node LoadNode(std::istream& stream);
    };

    struct PrintContext {
        PrintContext(std::ostream& out) : out(out) {}

        PrintContext(std::ostream& out, int indent_step, int indent = 0) : out(out), indent_step(indent_step), indent(indent) {}

        PrintContext Indented() const {
            return {out, indent_step, indent + indent_step};
        }

        void RenderIndent() const {
            for (int i = 0; i < indent; ++i) {
                out.put(' ');
            }
        }

        std::ostream& out;
        int indent_step = 4;
        int indent = 0;
    };

    class NodePrinter {
        const int def_indent_step = 4;
        const int def_indent = 4;

    public:
        NodePrinter(std::ostream& out_stream, bool pretty_print = true)
            : context_{out_stream, pretty_print ? NodePrinter::def_indent_step : 0, pretty_print ? def_indent : 0} {}

        template <typename Node, detail::EnableIfSame<Node, json::Node> = true>
        void PrintValue(Node&& value) const;

        template <typename Value, detail::EnableIfConvertible<Value, json::Node::ValueType> = true>
        void PrintValue(Value&& value) const;

        void operator()(std::nullptr_t) const;

        void operator()(bool value) const;

        void operator()(int value) const;

        void operator()(double value) const;

        template <typename String = std::string, detail::EnableIfSame<String, std::string> = true>
        void operator()(String&& value) const;

        template <typename Dict = json::Dict, detail::EnableIfSame<Dict, json::Dict> = true>
        void operator()(Dict&& dict) const;

        template <typename Array = json::Array, detail::EnableIfSame<Array, json::Array> = true>
        void operator()(Array&& array) const;

    private:
        PrintContext context_;
    };

    class Document {
    public:
        template <typename Node, detail::EnableIfConvertible<Node, json::Node> = true>
        explicit Document(Node&& root) : root_(std::forward<Node>(root)) {}

        const Node& GetRoot() const;

        Node& GetRoot() {
            return root_;
        }

        bool operator==(const Document& rhs) const {
            return this == &rhs || root_ == rhs.root_;
        }

        bool operator!=(const Document& rhs) const {
            return !(*this == rhs);
        }

        void Print(std::ostream& output) const;

        static void Print(const Document& doc, std::ostream& output);

        static Document Load(std::istream& stream);

    private:
        Node root_;
    };

    class Parser {
    public:
        class ParsingError : public std::runtime_error {    //!!!!!!!!!!
        public:
            using runtime_error::runtime_error;
        };

        struct Token {
            static constexpr const std::string_view TRUE_LITERAL{"true"};
            static constexpr const std::string_view FALSE_LITERAL{"false"};
            static constexpr const std::string_view NULL_LITERAL{"null"};
            static const char START_TRUE = 't';
            static const char START_FALSE = 'f';
            static const char START_NULL = 'n';
            static const char START_ARRAY = '[';
            static const char END_ARRAY = ']';
            static const char START_OBJ = '{';
            static const char END_OBJ = '}';
            static const char START_STRING = '"';
            static const char END_STRING = START_STRING;
            static const char VALUE_SEPARATOR = ',';
            static const char SIGN_LITERAL = '-';
            static const char DICT_SEPARATOR = ':';
        };

        const std::unordered_set<char> start_literals{Token::START_OBJ, Token::START_ARRAY,  Token::START_STRING, Token::START_TRUE,
                                                      Token::START_FALSE, Token::START_NULL,   Token::SIGN_LITERAL};
        const std::unordered_set<char> end_literals{Token::END_OBJ, Token::END_ARRAY,  Token::START_STRING, Token::START_TRUE,
                                                      Token::START_FALSE, Token::START_NULL,   Token::SIGN_LITERAL};

    public:
        explicit Parser(std::istream& input_stream) : input_(input_stream), numeric_parser_(input_), string_parser_(input_) {}

        Node Parse() const;

        bool ParseBool() const;

        std::nullptr_t ParseNull() const;

        Array ParseArray() const;

        Dict ParseDict() const;

        std::string ParseString() const;

        Numeric ParseNumber() const;

        void Ignore(const char character) const {
            static const std::streamsize max_count = std::numeric_limits<std::streamsize>::max();
            input_.ignore(max_count, character);
        }

        void Skip(const char character) const {
            char ch;

            if (input_ >> ch && ch != character) {
                input_.putback(ch);
                return;
            }
            for (; input_ >> ch && ch == character;) {
            }
        }

    private:
        class NumericParser {
        public:
            NumericParser(std::istream& input) : input_(input) {}

            Numeric Parse() const;

        private:
            std::istream& input_;

            // Считывает в parsed_num очередной символ из input
            void ReadChar(std::string& buffer) const;

            /// Считывает одну или более цифр в parsed_num из input
            void ReadDigits(std::string& buffer) const;
        };

        class StringParser {
        public:
            StringParser(std::istream& input) : input_(input) {}

            std::string Parse() const;

        private:
            std::istream& input_;
        };

    private:
        std::istream& input_;
        NumericParser numeric_parser_;
        StringParser string_parser_;
    };

    using ParsingError = Parser::ParsingError;

    Document Load(std::istream& input);

    void Print(const Document& doc, std::ostream& output);
}

namespace json /* Node class template impl */ {
    template <typename T, detail::EnableIfConvertible<T, NodeValueType>>
    bool Node::IsType() const {
        return std::holds_alternative<T>(*this);
    }

    template <typename T, detail::EnableIf<detail::IsConvertible<T, NodeValueType>::value || detail::IsSame<T, void>::value>>
    const auto& Node::GetValue() const {
        if constexpr (detail::IsConvertible<T, NodeValueType>::value == true) {
            /*try {
                return std::get<T>(*this);
            } catch (std::bad_variant_access const& ex) {
                throw std::logic_error(ex.what() + ": Node don't contained this type alternative"s);
            }*/
            auto* ptr = GetValuePtr<T>();
            if (ptr == 0) {
                throw std::logic_error("Node don't contained this type alternative");
            }
            return *ptr;
        } else {
            return static_cast<const Node::ValueType&>(*this);
        }
    }

    template <typename T, detail::EnableIf<detail::IsConvertible<T, NodeValueType>::value || detail::IsSame<T, void>::value>>
    const auto* Node::GetValuePtr() const noexcept {
        if constexpr (detail::IsConvertible<T, NodeValueType>::value == true) {
            return std::get_if<T>(this);
        } else {
            return static_cast<const Node::ValueType*>(this);
        }
    }
}

namespace json /* NodePrinter class template impl */ {
    template <typename Dict, detail::EnableIfSame<Dict, json::Dict>>
    void NodePrinter::operator()(Dict&& dict) const {
        int size = dict.size();
        context_.out << Parser::Token::START_OBJ;
        static const std::string sep{Parser::Token::VALUE_SEPARATOR};
        std::for_each(dict.begin(), dict.end(), [this, &size](const auto& item) {
            context_.out << Parser::Token::START_STRING << item.first << Parser::Token::END_STRING << Parser::Token::DICT_SEPARATOR << ' ';
            PrintValue(item.second);
            context_.out << (--size > 0 ? sep : "");
        });
        context_.out << Parser::Token::END_OBJ;
    }

    template <typename Array, detail::EnableIfSame<Array, json::Array>>
    void NodePrinter::operator()(Array&& array) const {
        int size = array.size();
        context_.out << Parser::Token::START_ARRAY;
        static const std::string sep{Parser::Token::VALUE_SEPARATOR};
        std::for_each(array.begin(), array.end(), [this, &size](const Node& node) {
            PrintValue(node);
            context_.out << (--size > 0 ? sep : "");
        });
        context_.out << Parser::Token::END_ARRAY;
    }

    template <typename String, detail::EnableIfSame<String, std::string>>
    void NodePrinter::operator()(String&& value) const {
        const char escape_symbol = '\\';
        context_.out << '"';
        for (char c : value) {
            switch (c) {
            case '\n':
                context_.out << escape_symbol << 'n';
                break;
            case '\r':
                context_.out << escape_symbol << 'r';
                break;
            case '\\':
                context_.out << escape_symbol << escape_symbol;
                break;
            case '\"':
                context_.out << escape_symbol << '"';
                break;
            default:
                context_.out << c;
            }
        }
        context_.out << '"';
    }

    template <typename Node, detail::EnableIfSame<Node, json::Node>>
    void NodePrinter::PrintValue(Node&& value) const {
        std::visit(*this, value.GetValue());
    }

    template <typename Value, detail::EnableIfConvertible<Value, json::Node::ValueType>>
    void NodePrinter::PrintValue(Value&& value) const {
        std::visit(*this, std::forward<Value>(value));
    }
}
