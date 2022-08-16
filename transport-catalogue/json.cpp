#include "json.h"

#include <cstddef>
#include <unordered_set>

using namespace std;

namespace json {

    Node LoadNode(std::istream& input) {
        return Parser(input).Parse();
    }

    Document Load(std::istream& input) {
        return Document::Load(input);
    }

    void Print(const Document& doc, std::ostream& output) {
        Document::Print(doc, output);
    }
}

namespace json /* Document */ {
    const Node& Document::GetRoot() const {
        return root_;
    }

    void Document::Print(std::ostream& output) const {
        auto node_json = GetRoot().GetValue();
        std::visit(NodePrinter{output}, node_json);
    }

    Document Document::Load(std::istream& stream) {
        return Document{Node::LoadNode(stream)};
    }

    void Document::Print(const Document& doc, std::ostream& output) {
        doc.Print(output);
    }
}

namespace json /* Parser */ {

    Node Parser::Parse() const {
        auto it = std::istream_iterator<char>(input_);
        auto end = std::istream_iterator<char>();
        if (it == end) {
            return {};
        }
        char ch;
        for (; it != end; ++it) {
            ch = *it;
            if (start_literals.count(ch) || std::isdigit(ch)) {
                // input_.putback(ch);
                break;
            }
        }
        if (it == end) {
            // Поток закончился до того, как встретили закрывающую кавычку?
            throw ParsingError("String parsing error");
        }
        //! input_.seekg(skip_count);
        [[maybe_unused]] char yyyy = input_.peek();  //!
        /*char ch;
        input_ >> ch;
        for (; ch != input_.eof(); input_ >> ch) {
            if (start_literals.count(ch) || std::isdigit(ch)) {
                // input_.putback(ch);
                break;
            }
        }*/

        if (ch == Token::START_ARRAY) {
            Node result(ParseArray());
            Notify(result);
            return result;
        } else if (ch == Token::START_OBJ) {
            Node result(ParseDict());
            Notify(result);
            return result;
        } else if (ch == Token::START_STRING) {
            Node result(ParseString());
            Notify(result);
            return result;
        } else {
            input_.putback(ch);
            if (ch == Token::START_TRUE || ch == Token::START_FALSE) {
                Node result(ParseBool());
                Notify(result);
                return result;
            } else if (ch == Token::START_NULL) {
                Node result(ParseNull());
                Notify(result);
                return result;
            } else if (std::isdigit(ch) || ch == Token::SIGN_LITERAL) {
                const Numeric num = ParseNumber();
                if (holds_alternative<int>(num)) {
                    Node result(get<int>(num));
                    Notify(result);
                    return result;
                } else {
                    Node result(get<double>(num));
                    Notify(result);
                    return result;
                }
            }
        }
        throw ParsingError("Parsing error");
    }

    bool Parser::ParseBool() const {
        assert(input_.peek() == Token::START_TRUE || input_.peek() == Token::START_FALSE);

        const bool is_true_literal = input_.peek() == Token::START_TRUE;
        const std::string_view expected_literal = is_true_literal ? Token::TRUE_LITERAL : Token::FALSE_LITERAL;

        for (auto ptr = expected_literal.begin(); ptr != expected_literal.end(); ++ptr) {
            if (*ptr != input_.get()) {
                throw ParsingError("Boolean value parsing error");
            }
        }

        return is_true_literal;
    }

    std::nullptr_t Parser::ParseNull() const {
        assert(input_.peek() == Token::START_NULL);
        const std::string_view expected_literal = Token::NULL_LITERAL;

        for (auto ptr = expected_literal.begin(); ptr != expected_literal.end(); ++ptr) {
            if (*ptr != input_.get()) {
                throw ParsingError("Null value parsing error");
            }
        }

        return nullptr;
    }

    Array Parser::ParseArray() const {
        using namespace std::string_literals;

        Array result;
        char ch;

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

    Dict Parser::ParseDict() const {
        using namespace std::string_literals;

        Dict result;

        auto ptr = std::istream_iterator<char>(input_);
        auto end = std::istream_iterator<char>();
        if (ptr == end) {
            throw ParsingError("Dict parsing error. Stream is empty");
        }
        char ch = *ptr;
        for (; ptr != end && ch != Token::END_OBJ; ++ptr, ch = *ptr) {
            if (ch != Token::START_STRING) {
                // input_ >> ch;
                continue;
            }

            const std::string key = ParseString();
            Ignore(Token::KEYVAL_SEPARATOR);
            /*if (input_.peek() == input_.eof()) {
                throw ParsingError("Dict parsing error. Not found dictionary key/value separator character : ["s + Token::KEYVAL_SEPARATOR + "]"s);
            }*/
            result.emplace(move(key), Parse());

            [[maybe_unused]] char ppp = +input_.peek();  //!!!!!!!!!
            ppp = input_.peek();                         //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        }

        if (ch != Token::END_OBJ) {
            throw ParsingError("Dict parsing error. Not found dictionary close brace");
        }

        return result;
    }

    std::string Parser::ParseString() const {
        return string_parser_.Parse();
    }

    Numeric Parser::ParseNumber() const {
        return numeric_parser_.Parse();
    }
}

namespace json /* Parser::NumericParser */ {

    Numeric Parser::NumericParser::Parse() const {
        using namespace std::literals;
        std::string buffer;

        // Считывает в parsed_num очередной символ из input
        auto read_char = [this, &buffer] {
            ReadChar(buffer);
        };

        // Считывает одну или более цифр в parsed_num из input
        auto read_digits = [this, &buffer] {
            ReadDigits(buffer);
        };

        if (input_.peek() == '-') {
            read_char();
        }

        // Парсим целую часть числа
        if (input_.peek() == '0') {
            read_char();
            // После 0 в JSON не могут идти другие цифры
        } else {
            read_digits();
        }

        bool is_integer = true;
        // Парсим дробную часть числа
        if (input_.peek() == '.') {
            read_char();
            read_digits();
            is_integer = false;
        }

        // Парсим экспоненциальную часть числа
        if (int ch = input_.peek(); ch == 'e' || ch == 'E') {
            read_char();
            if (ch = input_.peek(); ch == '+' || ch == '-') {
                read_char();
            }
            read_digits();
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

    void Parser::NumericParser::ReadChar(std::string& buffer) const {
        using namespace std::string_literals;

        buffer += static_cast<char>(input_.get());
        if (!input_) {
            throw ParsingError("Failed to read number from stream"s);
        }
    }

    void Parser::NumericParser::ReadDigits(std::string& buffer) const {
        using namespace std::literals;
        if (!std::isdigit(input_.peek())) {
            throw ParsingError("A digit is expected"s);
        }
        while (std::isdigit(input_.peek())) {
            ReadChar(buffer);
        }
    }
}

namespace json /* Parser::StringParser */ {

    std::string Parser::StringParser::Parse() const {
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
                input_.putback(ch);  //!!!!!!!!!!!!!!
                std::string line;
                std::getline(input_, line, '\r');
                throw ParsingError("Unexpected end of line"s);
            } else {
                // Просто считываем очередной символ и помещаем его в результирующую строку
                s.push_back(ch);
            }
            ++it;
        }

        return s;
    }
}

namespace json /* NodePrinter */ {

    void NodePrinter::operator()(std::nullptr_t) const {
        context_.out << Parser::Token::NULL_LITERAL;
    }

    void NodePrinter::operator()(bool value) const {
        context_.out << std::boolalpha << value;
    }

    void NodePrinter::operator()(int value) const {
        context_.out << value;
    }

    void NodePrinter::operator()(double value) const {
        context_.out << value;
    }
}

namespace json /* Node */ {
    bool Node::IsNull() const {
        return /*IsType<std::monostate>() ||*/ IsType<std::nullptr_t>();
    }

    bool Node::IsBool() const {
        return IsType<bool>();
    }

    bool Node::IsInt() const {
        return IsType<int>();
    }

    bool Node::IsDouble() const {
        return IsType<double>() || IsType<int>();
    }

    bool Node::IsPureDouble() const {
        return IsType<double>();
    }

    bool Node::IsString() const {
        return IsType<std::string>();
    }

    bool Node::IsArray() const {
        return IsType<json::Array>();
    }

    bool Node::IsMap() const {
        return IsType<json::Dict>();
    }

    const Node::ValueType* Node::GetValuePtr() const {
        return GetValuePtr<void>();
    }

    bool Node::AsBool() const {
        return GetValue<bool>();
    }

    int Node::AsInt() const {
        return GetValue<int>();
    }

    double Node::AsDouble() const {
        const double* double_ptr = GetValuePtr<double>();
        if (double_ptr != nullptr) {
            return *double_ptr;
        }
        const int* int_ptr = GetValuePtr<int>();
        if (int_ptr != nullptr) {
            return *int_ptr;
        }
        throw std::logic_error("Node don't contained [double/integer] value alternative.");
    }

    const std::string& Node::AsString() const {
        return GetValue<std::string>();
    }

    const json::Array& Node::AsArray() const {
        return GetValue<json::Array>();
    }

    const json::Dict& Node::AsMap() const {
        return GetValue<json::Dict>();
    }

    bool Node::operator==(const Node& rhs) const {
        return this == &rhs || *GetValuePtr() == *rhs.GetValuePtr();
    }

    bool Node::operator!=(const Node& rhs) const {
        return !(*this == rhs);
    }

    Node Node::LoadNode(std::istream& input) {
        return Parser(input).Parse();
    }
}