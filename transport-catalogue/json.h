#pragma once

#include <cassert>
#include <cstddef>
#include <iostream>
#include <istream>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>
#include <limits>

namespace json::detail {
    template <bool Condition>
    using EnableIf = typename std::enable_if_t<Condition, bool>;

    template <typename FromType, typename ToType>
    using EnableIfConvertible = std::enable_if_t<std::is_convertible_v<std::decay_t<FromType>, ToType>, bool>;

    template <typename FromType, typename ToType>
    using IsConvertible = std::is_convertible<std::decay_t<FromType>, ToType>;

    template <typename FromType, typename ToType>
    using IsSame = std::is_same<std::decay_t<FromType>, ToType>;

    template <typename FromType, typename ToType>
    using EnableIfSame = std::enable_if_t<std::is_same_v<std::decay_t<FromType>, ToType>, bool>;

    template <typename BaseType, typename DerivedType>
    using EnableIfBaseOf = std::enable_if_t<std::is_base_of_v<BaseType, std::decay_t<DerivedType>>, bool>;
}

namespace json {
    class Node;
    using Array = std::vector<Node>;
    using Dict = std::map<std::string, Node>;
    // template <typename ...T>
    // using NodeValueVarianrs = ...T;
    using NodeValueType = std::variant<std::nullptr_t, /*std::monostate,*/ std::string, int, double, bool, json::Array, json::Dict>;
    using Numeric = std::variant<int, double>;

    class Node : private NodeValueType {
    public:
        using ValueType = variant;

        template <typename ValueType, detail::EnableIfConvertible<ValueType, Node::ValueType> = true>
        Node(ValueType&& value) : NodeValueType(std::forward<ValueType>(value)) {}

        Node() : NodeValueType(nullptr) {}

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
        const auto* GetValuePtr() const noexcept;

        const Node::ValueType* GetValuePtr() const;

        bool AsBool() const;

        int AsInt() const;

        double AsDouble() const;

        const std::string& AsString() const;

        const json::Array& AsArray() const;

        const json::Dict& AsMap() const;

        bool operator==(const Node& rhs) const;

        bool operator!=(const Node& rhs) const;
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
        // Контекст вывода, хранит ссылку на поток вывода и текущий отсуп

        NodePrinter(std::ostream& out_stream, bool pretty_print = true)
            : context_{out_stream, pretty_print ? NodePrinter::def_indent_step : 0, pretty_print ? def_indent : 0} {}

        template <typename Value>
        void PrintValue(Value&& value) const;

        void operator()(std::nullptr_t) const;

        void operator()(Array array) const;

        void operator()(bool value) const;

        void operator()(int value) const;

        void operator()(double value) const;

        template <typename String = std::string, detail::EnableIfSame<String, std::string> = true>
        void operator()(String&& value) const;

        template <typename Dict = json::Dict, detail::EnableIfSame<Dict, json::Dict> = true>
        void operator()(Dict&& dict) const;

    private:
        PrintContext context_;
    };

    class Document {
    public:
        template <typename Node, detail::EnableIfConvertible<Node, json::Node> = true>
        explicit Document(Node&& root) : root_(std::forward<Node>(root)) {
        }

        const Node& GetRoot() const;

        bool operator==(const Document& rhs) const {
            return this == &rhs || root_ == rhs.root_;
        }

        bool operator!=(const Document& rhs) const {
            return !(*this == rhs);
        }

    private:
        Node root_;
    };

    class Parser {
    public:
        class ParsingError : public std::runtime_error {
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

    public:
        explicit Parser(std::istream& input_stream) : input_(input_stream), numeric_parser_(input_), string_parser_(input_) {}

        Node Parse() const;

        Node ParseBool() const;

        Node ParseNull() const;

        Node ParseArray() const;

        Node ParseDict() const;

        Node ParseString() const;

        Node ParseNumber() const;

        void Ignore(const char character) const {
            /*size_t size = 0;
            for (char ch; input_ >> ch && ch == character; ++size) {
            }
            return size;*/
            static const std::streamsize max_count = std::numeric_limits<std::streamsize>::max();
            input_.ignore(max_count, character);
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
        using namespace std::string_literals;
        if constexpr (detail::IsConvertible<T, NodeValueType>::value == true) {
            try {
                return std::get<T>(*this);
            } catch (std::bad_variant_access const& ex) {
                throw std::logic_error(ex.what() + ": Node don't contained this type alternative."s);
            }
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
        int size = dict.size() - 1;
        context_.out << '{';
        for (const auto& [key, node] : dict) {
            context_.out << '"' << key << "\": ";
            PrintValue(node);
            if (size > 0) {
                context_.out << ',';
                --size;
            }
        }
        context_.out << '}';
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

    template <typename Value>
    void NodePrinter::PrintValue(Value&& value) const {
        std::visit(*this, value.GetValue());
    }
}
