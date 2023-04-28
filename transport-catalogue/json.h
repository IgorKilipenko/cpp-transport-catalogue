#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <exception>
#include <functional>
#include <iostream>
#include <istream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace json::detail /* template helpers */ {
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

namespace json /* Interfaces */ {

    template <typename Data>
    class INotifier {
        virtual void AddListener(
            const void* listener, const std::function<void(const Data&, const void* sender)> on_data,
            std::optional<const std::function<void(const std::exception&, const void* sender)>> on_error = std::nullopt) = 0;
        virtual void RemoveListener(const void* listener) = 0;

    protected:
        virtual void Notify(const Data&) const = 0;
    };
}

namespace json /* Node */ {
    class Node;
    using ArrayBase = std::vector<Node>;
    using DictBase = std::map<std::string, Node, std::less<>>;

    class Array : public ArrayBase {
        using ArrayBase::vector;

    public:
        Array(const Array& other) = default;
        Array& operator=(const Array& other) = default;
        Array(Array&& other) = default;
        Array& operator=(Array&& other) = default;
    };

    using Numeric = std::variant<int, double>;

    class Dict : public DictBase {
        using DictBase::map;

    public:
        using ValueType = DictBase::mapped_type;
        using ItemType = DictBase::value_type;
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

    public:
        Dict(const Dict& other) = default;
        Dict& operator=(const Dict& other) = default;
        Dict(Dict&& other) = default;
        Dict& operator=(Dict&& other) = default;
    };

    using NodeValueType = std::variant<std::nullptr_t, std::string, int, double, bool, json::Array, json::Dict>;

    class Node : private NodeValueType {
    private:
        static double EQUALITY_TOLERANCE;

    public:
        using NodeValueType::variant;
        using ValueType = NodeValueType;
        using OnNodeItemParsedCallback = std::function<void(const Node&, const void*)>;

    public:
        static double GetEqualityTolerance();
        static void SetEqualityTolerance(double tolerance);

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
        auto& GetValue();

        template <typename T = void, detail::EnableIf<detail::IsConvertible<T, NodeValueType>::value || detail::IsSame<T, void>::value> = true>
        auto&& ExtractValue();

        template <typename T = void, detail::EnableIf<detail::IsConvertible<T, NodeValueType>::value || detail::IsSame<T, void>::value> = true>
        const auto* GetValuePtr() const noexcept;

        template <typename T = void, detail::EnableIf<detail::IsConvertible<T, NodeValueType>::value || detail::IsSame<T, void>::value> = true>
        auto* GetValuePtr() noexcept;

        const Node::ValueType* GetValuePtr() const;

        Node::ValueType* GetValuePtr();

        bool AsBool() const;

        int AsInt() const;

        double AsDouble() const;

        const std::string& AsString() const;

        std::string& AsString();

        std::string&& ExtractString();

        const json::Array& AsArray() const;

        json::Array& AsArray();

        json::Array&& ExtractArray();

        const json::Dict& AsMap() const;

        json::Dict& AsMap();

        json::Dict&& ExtractMap();

        bool operator==(const Node& rhs) const;

        bool operator!=(const Node& rhs) const;

        bool EqualsWithTolerance(const Node& rhs, double tolerance = EQUALITY_TOLERANCE) const;

        static Node LoadNode(std::istream& stream, const OnNodeItemParsedCallback* = nullptr);

        template <typename InputStream, detail::EnableIf<detail::IsBaseOfV<std::istream, InputStream> && !std::is_reference_v<InputStream>> = true>
        static Node LoadNode(InputStream&& stream, const OnNodeItemParsedCallback* = nullptr);

        void Print(std::ostream& output, bool pretty_print = true) const;

        static void Print(const ValueType& value, std::ostream& output, bool pretty_print = true);

        template <typename Printer, detail::EnableIf<!std::is_reference_v<Printer>> = true>
        void Print(Printer&& printer) const;
    };
}

namespace json /* Document */ {
    class Document {
    public:
        template <typename Node, detail::EnableIfConvertible<Node, json::Node> = true>
        explicit Document(Node&& root) : root_(std::forward<Node>(root)) {}

        const Node& GetRoot() const;

        Node& GetRoot();

        bool operator==(const Document& rhs) const;

        bool operator!=(const Document& rhs) const;

        void Print(std::ostream& output, bool pretty_print = true) const;

        template <typename Printer, detail::EnableIf<!std::is_reference_v<Printer>> = true>
        void Print(Printer&& printer) const;

        static void Print(const Document& doc, std::ostream& output);

        static Document Load(std::istream& stream);

        template <typename InputStream, detail::EnableIf<detail::IsBaseOfV<std::istream, InputStream> && !std::is_reference_v<InputStream>> = true>
        static Document Load(InputStream&& stream);

    private:
        Node root_;
    };
}

namespace json /* Node Printer */ {
    struct PrintContext {
    public:
        PrintContext(std::ostream& out) : out(out) {}

        PrintContext(std::ostream& out, uint8_t indent_step, size_t indent = 0) : out(out), indent_step(indent_step), indent(indent) {}

        PrintContext Indented(bool backward = false) const;

        void RenderIndent(bool backward = false) const;

        void RenderNewLine() const;

        std::string NewLineSymbols() const;

        template <typename Value>
        std::ostream& Print(Value&& value) const;

        std::ostream& Stream() const;

        uint8_t GetStep() const;

        size_t GetCurrentIndent() const;

        bool IsPretty() const;

    public:
        std::ostream& out;

    private:
        uint8_t indent_step = 4;
        size_t indent = 0;
        const std::string new_line_ = "\n";
    };

    class NodePrinter {
        const uint8_t def_indent_step = 4;

    public:
        NodePrinter(std::ostream& out_stream, bool pretty_print = true)
            : context_{out_stream, pretty_print ? def_indent_step : uint8_t{0}, pretty_print ? def_indent_step : uint8_t{0}} {}

        NodePrinter(PrintContext&& context) : context_{context} {}

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

        NodePrinter NextIndent() const {
            return !context_.IsPretty() ? *this : NodePrinter(context_.Indented());
        }

    private:
        PrintContext context_;
    };
}

namespace json /* Parser */ {
    class Parser : public INotifier<Node> {
    public:
        class ParsingError : public std::runtime_error {  //!
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
            static const char KEYVAL_SEPARATOR = ':';
        };

    public:
        explicit Parser(std::istream& input_stream) : input_(input_stream), numeric_parser_(input_), string_parser_(input_), is_broadcast_{false} {}

        Node Parse() const;

        bool ParseBool() const;

        std::nullptr_t ParseNull() const;

        Array ParseArray() const;

        Dict ParseDict() const;

        std::string ParseString() const;

        Numeric ParseNumber() const;

        void Ignore(const char character) const;

        void AddListener(
            const void* listener, const std::function<void(const Node&, const void*)> on_data,
            std::optional<const std::function<void(const std::exception&, const void*)>> /*on_error*/ = nullptr) override;

        void RemoveListener(const void* /*listener*/) override;

    protected:
        bool HasListeners() const;

        void Notify(const Node& node) const noexcept override;

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
        const void* listener_ = nullptr;
        const std::function<void(const Node&, const void*)>* listener_on_data;
        bool is_broadcast_ = false;
    };

    using ParsingError = Parser::ParsingError;

    Document Load(std::istream& input);

    void Print(const Document& doc, std::ostream& output);
}

namespace json /* Node class template implementation */ {
    template <typename T, detail::EnableIfConvertible<T, NodeValueType>>
    bool Node::IsType() const {
        return std::holds_alternative<T>(*this);
    }

    template <typename T, detail::EnableIf<detail::IsConvertible<T, NodeValueType>::value || detail::IsSame<T, void>::value>>
    auto& Node::GetValue() {
        if constexpr (detail::IsConvertible<T, NodeValueType>::value == true) {
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
    const auto& Node::GetValue() const {
        if constexpr (detail::IsConvertible<T, NodeValueType>::value == true) {
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

    template <typename T, detail::EnableIf<detail::IsConvertible<T, NodeValueType>::value || detail::IsSame<T, void>::value>>
    auto* Node::GetValuePtr() noexcept {
        if constexpr (detail::IsConvertible<T, NodeValueType>::value == true) {
            return std::get_if<T>(this);
        } else {
            return static_cast<Node::ValueType*>(this);
        }
    }

    template <typename T, detail::EnableIf<detail::IsConvertible<T, NodeValueType>::value || detail::IsSame<T, void>::value>>
    auto&& Node::ExtractValue() {
        if constexpr (detail::IsConvertible<T, NodeValueType>::value == true) {
            if (!std::holds_alternative<T>(*this)) {
                throw std::logic_error("Node don't contained this type alternative");
            }
            return std::get<T>(std::move(*this));
        } else {
            return static_cast<Node::ValueType&&>(std::move(*this));
        }
    }

    template <typename Printer, detail::EnableIf<!std::is_reference_v<Printer>>>
    void Node::Print(Printer&& printer) const {
        std::visit(std::forward<Printer>(printer), GetValue());
    }

    template <typename InputStream, detail::EnableIf<detail::IsBaseOfV<std::istream, InputStream> && !std::is_reference_v<InputStream>>>
    Node Node::LoadNode(InputStream&& stream, const OnNodeItemParsedCallback* on_load) {
        return LoadNode(stream, on_load);
    }
}

namespace json /* NodePrinter class template implementation */ {
    template <typename Dict, detail::EnableIfSame<Dict, json::Dict>>
    void NodePrinter::operator()(Dict&& dict) const {
        int size = dict.size();

        context_.out << Parser::Token::START_OBJ;

        static const std::string sep{Parser::Token::VALUE_SEPARATOR};

        std::for_each(dict.begin(), dict.end(), [this, &size](const auto& item) {
            context_.RenderNewLine();
            context_.RenderIndent();
            context_.out << Parser::Token::START_STRING << item.first << Parser::Token::END_STRING << Parser::Token::KEYVAL_SEPARATOR << ' ';
            PrintValue(item.second);
            context_.out << (--size > 0 ? sep : "");
        });
        context_.RenderNewLine();
        context_.RenderIndent(true);
        context_.out << Parser::Token::END_OBJ;
    }

    template <typename Array, detail::EnableIfSame<Array, json::Array>>
    void NodePrinter::operator()(Array&& array) const {
        int size = array.size();

        context_.out << Parser::Token::START_ARRAY;

        static const std::string sep{Parser::Token::VALUE_SEPARATOR};

        std::for_each(array.begin(), array.end(), [this, &size](const Node& node) {
            context_.RenderNewLine();
            context_.RenderIndent();
            PrintValue(node);
            context_.out << (--size > 0 ? sep : "");
        });

        context_.RenderNewLine();
        context_.RenderIndent(true);
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
        std::visit(this->NextIndent(), value.GetValue());
    }

    template <typename Value, detail::EnableIfConvertible<Value, json::Node::ValueType>>
    void NodePrinter::PrintValue(Value&& value) const {
        std::visit(this->NextIndent(), std::forward<Value>(value));
    }

    template <typename Value>
    std::ostream& PrintContext::Print(Value&& value) const {
        out << std::forward<Value>(value);
        return out;
    }
}

namespace json /* Document class template implementation */ {
    template <typename Printer, detail::EnableIf<!std::is_reference_v<Printer>>>
    void Document::Print(Printer&& printer) const {
        root_.Print(printer);
    }

    template <typename InputStream, detail::EnableIf<detail::IsBaseOfV<std::istream, InputStream> && !std::is_reference_v<InputStream>>>
    Document Document::Load(InputStream&& stream) {
        return Load(stream);
    }
}