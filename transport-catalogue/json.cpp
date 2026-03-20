#include <iostream>
#include <variant>
#include <string_view>
#include <algorithm>
#include <string>

#include "json.h"


using namespace std;

namespace json {

    namespace {

        Node LoadNode(istream& input);

        using Number = std::variant<nullptr_t, int, double>;

        bool IsOneOf(const char& c, string_view elements) {
            return std::ranges::any_of(elements, [&](const char& ch) {return c == ch;});
        }

        Number ReadNumber(std::istream& input) {
            using namespace std::literals;

            std::string parsed_num;

            auto read_char = [&parsed_num, &input] {
                parsed_num += static_cast<char>(input.get());
                if (!input) {
                    throw ParsingError("Failed to read number from stream"s);
                }
                };

            auto read_digits = [&input, read_char] {
                if (!std::isdigit(input.peek())) {
                    throw ParsingError("A digit is expected"s);
                }
                while (std::isdigit(input.peek())) {
                    read_char();
                }
                };

            if (input.peek() == '-') {
                read_char();
            }

            if (input.peek() == '0') {
                read_char();
            }
            else {
                read_digits();
            }

            bool is_int = true;

            if (input.peek() == '.') {
                read_char();
                read_digits();
                is_int = false;
            }

            if (char ch = input.peek(); ch == 'e' || ch == 'E') {
                read_char();
                if (ch = input.peek(); ch == '+' || ch == '-') {
                    read_char();
                }
                read_digits();
                is_int = false;
            }

            try {
                if (is_int) {
                    try {
                        return std::stoi(parsed_num);
                    }
                    catch (...) {
                        // В случае неудачи, например, при переполнении,
                        // код ниже попробует преобразовать строку в double
                    }
                }
                return std::stod(parsed_num);
            }
            catch (...) {
                throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
            }
        }

        Node LoadInt(istream& input) {
            variant<nullptr_t, int, double> number = ReadNumber(input);
            if (holds_alternative<nullptr_t>(number)) {
                throw ParsingError("Failed to read number");
            }

            if (holds_alternative<int>(number)) {
                return Node(get<int>(number));
            }

            if (holds_alternative<double>(number)) {
                return Node(get<double>(number));
            }

            throw ParsingError("Number parsing error");
        }

        Node LoadString(std::istream& input) {
            using namespace std::literals;

            auto it = std::istreambuf_iterator<char>(input);
            auto end = std::istreambuf_iterator<char>();
            std::string s;
            while (true) {
                if (it == end) {
                    throw ParsingError("String parsing error");
                }
                const char ch = *it;
                if (ch == '"') {
                    ++it;
                    break;
                }
                else if (ch == '\\') {
                    ++it;
                    if (it == end) {
                        throw ParsingError("String parsing error");
                    }
                    const char escaped_char = *(it);
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
                        throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
                    }
                }
                else if (ch == '\n' || ch == '\r') {
                    throw ParsingError("Unexpected end of line"s);
                }
                else {

                    s.push_back(ch);
                }
                ++it;
            }

            return Node{ s };
        }

        Node LoadArray(istream& input) {
            Array result;
            char c;

            while (input >> c && c != ']') {
                if (c != ',') {
                    input.putback(c);
                }
                result.emplace_back(LoadNode(input));
            }

            if (!input) {
                throw ParsingError("Array parsing error"s);
            }

            return Node(move(result));
        }

        Node LoadDict(istream& input) {
            Dict result;
            char c;

            while (input >> c && c != '}') {
                if (c == '"') {
                    string key = LoadString(input).AsString();
                    if (input >> c && c == ':') {
                        if (result.find(key) != result.end()) {
                            throw ParsingError("Duplicate key '"s + key + "' have been found");
                        }
                        result.emplace(key, LoadNode(input));
                    }
                    else {
                        throw ParsingError(": is expected but '"s + c + "' has been found"s);
                    }
                }
                else if (c != ',') {
                    throw ParsingError(R"(',' is expected but ')"s + c + "' has been found"s);
                }
            }

            if (!input) {
                throw ParsingError("Dictionary parsing error"s);
            }

            return Node(move(result));
        }

        Node LoadBool(istream& input) {
            char c;
            string str;
            input >> c;
            if (c == 't') {
                str += c;
                for (size_t i = 0; i < 3; ++i) {
                    input >> c;
                    str += c;
                }
            }
            if (c == 'f') {
                str += c;
                for (size_t i = 0; i < 4; ++i) {
                    input >> c;
                    str += c;
                }
            }

            char next_c;
            if (input >> next_c && !IsOneOf(next_c, "]}\t\n\r\",")) {
                throw ParsingError("Incorrect bool type");
            }
            else {
                input.putback(next_c);
            }

            if (str == "true") {
                return Node(true);
            }
            if (str == "false") {
                return Node(false);
            }
            throw ParsingError("Incorrect bool type");
        }

        Node LoadNull(istream& input) {
            string str = "n";
            char c;
            for (int i = 0; i < 3; ++i) {
                c = ' ';
                input >> c;
                str += c;
            }
            if (str != "null") {
                throw ParsingError("Incomplete null type");
            }

            char next_c;
            if (input >> next_c && !IsOneOf(next_c, "]}\t\n\r\",")) {
                throw ParsingError("Incorrect null type");
            }
            else {
                input.putback(next_c);
            }
            return Node(nullptr);
        }

        Node LoadNode(istream& input) {

            char c;
            input >> c;

            if (c == '[') {
                return LoadArray(input);
            }
            else if (c == '{') {
                return LoadDict(input);
            }
            else if (c == '"') {
                return LoadString(input);
            }
            else if (c == 't' || c == 'f') {
                input.putback(c);
                return LoadBool(input);
            }
            else if (c == 'n') {
                return LoadNull(input);
            }
            else if (isdigit(c) || c == '-') {
                input.putback(c);
                return LoadInt(input);
            }
            else {
                throw ParsingError("Unrecognized character");
            }

            if (input.peek() != EOF) {
                throw ParsingError("Node parsing error");
            }
        }

    }  // namespace

    bool Node::IsInt() const {
        return holds_alternative<int>(*this);
    }

    bool Node::IsDouble() const {
        return holds_alternative<int>(*this) || holds_alternative<double>(*this);
    }

    bool Node::IsPureDouble() const {
        return holds_alternative<double>(*this);
    }

    bool Node::IsBool() const {
        return holds_alternative<bool>(*this);
    }

    bool Node::IsString() const {
        return holds_alternative<string>(*this);
    }

    bool Node::IsNull() const {
        return holds_alternative<nullptr_t>(*this);
    }

    bool Node::IsArray() const {
        return holds_alternative<Array>(*this);
    }

    bool Node::IsDict() const {
        return holds_alternative<Dict>(*this);
    }

    const NodeData& Node::GetData() const {
        return *this;
    }

    const Array& Node::AsArray() const
    {
        if (!this->IsArray()) {
            throw logic_error("Node is not Array");
        }
        return get<Array>(*this);
    }

    Array& Node::AsArray()
    {
        if (!this->IsArray()) {
            throw logic_error("Node is not Array");
        }
        return get<Array>(*this);
    }

    const Dict& Node::AsDict() const {
        if (!this->IsDict()) {
            throw logic_error("Node is not Map");
        }
        return get<Dict>(*this);
    }

    Dict& Node::AsDict() {
        if (!this->IsDict()) {
            throw logic_error("Node is not Map");
        }
        return get<Dict>(*this);
    }

    int Node::AsInt() const {
        if (!this->IsInt()) {
            throw logic_error("Node is not int");
        }
        return get<int>(*this);
    }

    bool Node::AsBool() const {
        if (!this->IsBool()) {
            throw logic_error("Node is not bool");
        }
        return get<bool>(*this);
    }

    double Node::AsDouble() const {
        if (!this->IsDouble()) {
            throw logic_error("Node is not double");
        }
        if (holds_alternative<int>(*this)) {
            return static_cast<double>(get<int>(*this));
        }
        return get<double>(*this);
    }

    const string& Node::AsString() const {
        if (!this->IsString()) {
            throw logic_error("Node is not string");
        }
        return get<string>(*this);
    }

    Document::Document(Node root)
        : root_(move(root)) {
    }

    const Node& Document::GetRoot() const {
        return root_;
    }

    void Node::Clear() {
        *this = nullptr;
    }

    void Document::Clear() {
        root_.Clear();
    }

    Document Load(istream& input) {
        try {
            return Document{ LoadNode(input) };
        }
        catch (...) {
            throw ParsingError("Invalid JSON document");
        }
    }

    struct PrintContext {
        std::ostream& out;
        int indent_step = 4;
        int indent = 0;

        void PrintIndent() const {
            for (int i = 0; i < indent; ++i) {
                out.put(' ');
            }
        }
        PrintContext Indented() const {
            return { out, indent_step, indent_step + indent };
        }
    };

    void PrintNode(const Node& node, const PrintContext& ctx);

    void PrintString(std::string str, const PrintContext& ctx) {
        ctx.out << "\"";
        for (size_t i = 0; i < str.size(); ++i) {
            switch (str[i]) {
            case '\\':
                ctx.out << "\\\\";
                break;
            case '\n':
                ctx.out << "\\n";
                break;
            case '\r':
                ctx.out << "\\r";
                break;
            case '\"':
                ctx.out << "\\\"";
                break;
            case '\t':
                ctx.out << "\\t";
                break;
            default:
                ctx.out << str[i];
            }
        }
        ctx.out << "\"";
    }

    template <typename Value>
    void PrintValue(const Value& value, const PrintContext& ctx) {
        ctx.out << value;
    }

    template <>
    void PrintValue<std::nullptr_t>(const std::nullptr_t&, const PrintContext& ctx) {
        ctx.out << "null"sv;
    }

    template <>
    void PrintValue<bool>(const bool& value, const PrintContext& ctx) {
        ctx.out << (value ? "true"sv : "false"sv);
    }

    template <>
    void PrintValue<std::string>(const std::string& value, const PrintContext& ctx) {
        PrintString(value, ctx);
    }    

    template <>
    void PrintValue<Array>(const Array& arr, const PrintContext& ctx) {
        ctx.PrintIndent();
        ctx.out << '[';
        ctx.out << '\n';
        auto indented_ctx = ctx.Indented();
        bool is_first = true;
        for (size_t i = 0; i < arr.size(); ++i) {
            if (is_first) {
                is_first = false;
            }
            else {
                ctx.out << ',' << '\n';
            }
            indented_ctx.PrintIndent();
            PrintNode(arr.at(i), indented_ctx);
        }
        ctx.out << '\n';
        ctx.PrintIndent();
        ctx.out << ']';
    }

    template <>
    void PrintValue<Dict>(const Dict& arr, const PrintContext& ctx) {
        ctx.out << '{';
        ctx.out << '\n';
        auto indented_ctx = ctx.Indented();
        bool is_first = true;
        for (const auto& [key, value] : arr) {
            if (is_first) {
                is_first = false;
            }
            else {
                ctx.out << ',' << '\n';
            }
            indented_ctx.PrintIndent();
            PrintString(key, indented_ctx);
            ctx.out << ':';
            PrintNode(value, indented_ctx);

        }
        ctx.out << '\n';
        ctx.PrintIndent();
        ctx.out << '}';
    }

    void PrintNode(const Node& node, const PrintContext& ctx) {
        std::visit(
            [&ctx](const auto& value) { PrintValue(value, ctx); },
            node.GetData());
    }

    void Print(const Document& doc, std::ostream& out) {
        PrintContext ctx(out, 2, 0);
        PrintNode(doc.GetRoot(), ctx);
    }

}  // namespace json