#pragma once

#include <cassert>
#include <cstddef>
#include <istream>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
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
    using NodeValueType = std::variant<std::nullptr_t, std::monostate, std::string, int, double, bool, json::Array, json::Dict>;
    using Numeric = std::variant<int, double>;

    class Node : private NodeValueType {
    public:
        using ValueType = NodeValueType;

        template <typename ValueType, detail::EnableIfConvertible<ValueType, Node::ValueType> = true>
        Node(ValueType&& value) : NodeValueType(std::move(value)) {}

        Node() : Node(nullptr) {}

        bool IsNull() const {
            return IsType<std::monostate>() || IsType<std::nullptr_t>();
        }

        bool IsBool() const {
            return IsType<bool>();
        }

        bool IsInt() const {
            return IsType<int>();
        }

        bool IsDouble() const {
            return IsType<double>() || IsType<int>();
        }

        bool IsPureDouble() const {
            return IsType<double>();
        }

        bool IsString() const {
            return IsType<std::string>();
        }

        bool IsArray() const {
            return IsType<json::Array>();
        }

        bool IsMap() const {
            return IsType<json::Dict>();
        }

        template <typename T, detail::EnableIfConvertible<T, NodeValueType> = true>
        bool IsType() const {
            return std::holds_alternative<T>(*this);
        }

        // template <typename T, detail::EnableIfConvertible<T, NodeValueType> = true>
        template <typename T = void, detail::EnableIf<detail::IsConvertible<T, NodeValueType>::value || detail::IsSame<T, void>::value> = true>
        const auto& GetValue() const {
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

        /*const Node::ValueType& GetValue() const {
            return GetValue<void>();
        }*/

        // template <typename T, detail::EnableIfConvertible<T, NodeValueType> = true>
        template <typename T = void, detail::EnableIf<detail::IsConvertible<T, NodeValueType>::value || detail::IsSame<T, void>::value> = true>
        const auto* GetValuePtr() const noexcept {
            if constexpr (detail::IsConvertible<T, NodeValueType>::value == true) {
                return std::get_if<T>(this);
            } else {
                return static_cast<const Node::ValueType*>(this);
            }
        }

        const Node::ValueType* GetValuePtr() const {
            return GetValuePtr<void>();
        }

        bool AsBool() const {
            return GetValue<bool>();
        }

        int AsInt() const {
            return GetValue<int>();
        }

        double AsDouble() const {
            const auto* ptr = GetValuePtr<double>();
            ptr = ptr == nullptr ? reinterpret_cast<const double*>(GetValuePtr<int>()) : ptr;
            if (ptr == nullptr) {
                throw std::logic_error("Node don't contained [double/integer] value alternative.");
            }
            return *ptr;
        }

        const std::string& AsString() const {
            return GetValue<std::string>();
        }

        const json::Array& AsArray() const {
            return GetValue<json::Array>();
        }

        const json::Dict& AsMap() const {
            return GetValue<json::Dict>();
        }

        bool operator==(const Node& rhs) const {
            return this == &rhs || *GetValuePtr() == *rhs.GetValuePtr();
        }

        bool operator!=(const Node& rhs) const {
            return !(*this == rhs);
        }
    };

    class Document {
    public:
        explicit Document(Node root);

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

    public:
        explicit Parser(std::istream& input_stream) : input_(input_stream), numeric_parser_(input_), string_parser_(input_) {}

        Node Parse() const {
            char c;
            input_ >> c;

            if (c && c == '[') {
                return ParseArray();
            } else if (c == Token::START_ARRAY) {
                return ParseDict();
            } else if (c == Token::START_STRING) {
                return ParseString();
            } else {
                input_.putback(c);
                if (c == Token::START_TRUE || c == Token::START_FALSE) {
                    return ParseBool();
                } else if (c == Token::START_NULL) {
                    return ParseNull();
                } else if (isdigit(c) || c == Token::SIGN_LITERAL) {
                    Numeric result = ParseNumber();
                    return {std::holds_alternative<double>(result) ? std::get<double>(std::move(result)) : std::get<int>(std::move(result))};
                }
            }
            throw ParsingError("Parsing error");
        }

        bool ParseBool() const {
            assert(input_.peek() == Token::START_TRUE || input_.peek() == Token::START_FALSE);

            const bool is_true_literal = input_.peek() == Token::START_TRUE;
            const std::string_view expected_literal = is_true_literal ? Token::TRUE_LITERAL : Token::FALSE_LITERAL;

            std::string result(expected_literal.size(), '\0');
            input_.getline(result.data(), result.size(), ' ');
            if (result != expected_literal) {
                throw ParsingError("Boolean value parsing error");
            }

            return is_true_literal;
        }

        std::nullptr_t ParseNull() const {
            assert(input_.peek() == Token::START_NULL);
            const std::string_view expected_literal = Token::NULL_LITERAL;

            std::string result(expected_literal.size(), '\0');
            input_.getline(result.data(), result.size(), ' ');
            if (result != expected_literal) {
                throw ParsingError("Null value parsing error");
            }
            return nullptr;
        }

        Array ParseArray() const {
            using namespace std::string_literals;

            Array result;
            char ch = '\0';

            for (; input_ >> ch && ch != Token::END_ARRAY;) {
                if (ch != Token::VALUE_SEPARATOR) {
                    input_.putback(ch);
                }
                result.push_back(Parse());
            }

            if (ch != Token::END_ARRAY) {
                throw ParsingError("Array parsing error"s);
            }

            return result;
        }

        Dict ParseDict() const {
            using namespace std::string_literals;

            Dict result;
            char ch = '\0';

            for (; input_ >> ch && ch != Token::END_OBJ;) {
                if (ch == Token::VALUE_SEPARATOR) {
                    input_ >> ch;
                }

                const std::string key = ParseString();
                input_ >> ch;
                result.emplace(move(key), Parse());
            }

            if (ch != Token::END_OBJ) {
                throw ParsingError("Dict parsing error"s);
            }

            return result;
        }

        std::string ParseString() const {
            return string_parser_.Parse();
        }

        Numeric ParseNumber() const {
            return numeric_parser_.Parse();
        }

    private:
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

        class NumericParser {
        public:
            NumericParser(std::istream& input) : input_(input) {}

            Numeric Parse() const {
                using namespace std::literals;
                std::string buffer;

                // Парсим целую часть числа
                if (const char ch = input_.peek(); ch == '-' || ch == '0' /* После 0 в JSON не могут идти другие цифры */) {
                    ReadChar(buffer);
                } else {
                    ReadDigits(buffer);
                }

                bool is_integer = true;
                // Парсим дробную часть числа
                if (input_.peek() == '.') {
                    ReadChar(buffer);
                    ReadDigits(buffer);
                    is_integer = false;
                }

                // Парсим экспоненциальную часть числа
                if (int ch = input_.peek(); ch == 'e' || ch == 'E') {
                    ReadChar(buffer);
                    if (ch = input_.peek(); ch == '+' || ch == '-') {
                        ReadChar(buffer);
                    }
                    ReadDigits(buffer);
                    is_integer = false;
                }

                try {
                    if (is_integer) {
                        // Сначала пробуем преобразовать строку в int
                        try {
                            return std::stoi(buffer);
                        } catch (...) {
                            // В случае неудачи, например, при переполнении,
                            // код ниже попробует преобразовать строку в double
                        }
                    }
                    return std::stod(buffer);
                } catch (...) {
                    throw ParsingError("Failed to convert "s + buffer + " to number"s);
                }
            }

        private:
            std::istream& input_;

            // Считывает в parsed_num очередной символ из input
            void ReadChar(std::string& buffer) const {
                using namespace std::string_literals;

                buffer += static_cast<char>(input_.get());
                if (!input_) {
                    throw ParsingError("Failed to read number from stream"s);
                }
            };

            /// Считывает одну или более цифр в parsed_num из input
            void ReadDigits(std::string& buffer) const {
                using namespace std::literals;
                if (!std::isdigit(input_.peek())) {
                    throw ParsingError("A digit is expected"s);
                }
                while (std::isdigit(input_.peek())) {
                    ReadChar(buffer);
                }
            };
        };

        class StringParser {
        public:
            StringParser(std::istream& input) : input_(input) {}
            std::string Parse() const {
                using namespace std::literals;

                auto it = std::istreambuf_iterator<char>(input_);
                auto end = std::istreambuf_iterator<char>();
                std::string s;
                while (true) {
                    if (it == end) {
                        // Поток закончился до того, как встретили закрывающую кавычку?
                        throw ParsingError("String parsing error");
                    }
                    const char ch = *it;
                    if (ch == '"') {
                        // Встретили закрывающую кавычку
                        ++it;
                        break;
                    } else if (ch == '\\') {
                        // Встретили начало escape-последовательности
                        ++it;
                        if (it == end) {
                            // Поток завершился сразу после символа обратной косой черты
                            throw ParsingError("String parsing error");
                        }
                        const char escaped_char = *(it);
                        // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
                        switch (escaped_char) {
                        case 'n':
                            s.push_back('\n');
                            break;
                        case 't':
                            s.push_back('\t');
                            break;
                        case 'r':
                            s.push_back('\r');
                            break;
                        case '"':
                            s.push_back('"');
                            break;
                        case '\\':
                            s.push_back('\\');
                            break;
                        default:
                            // Встретили неизвестную escape-последовательность
                            throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
                        }
                    } else if (ch == '\n' || ch == '\r') {
                        // Строковый литерал внутри- JSON не может прерываться символами \r или \n
                        throw ParsingError("Unexpected end of line"s);
                    } else {
                        // Просто считываем очередной символ и помещаем его в результирующую строку
                        s.push_back(ch);
                    }
                    ++it;
                }

                return s;
            }

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