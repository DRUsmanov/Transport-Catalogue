#pragma once

#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace json {

    class ParsingError : public std::runtime_error {
    public:
        using runtime_error::runtime_error;
    };

    class Node;

    using Dict = std::map<std::string, Node>;
    using Array = std::vector<Node>;
    using NodeData = std::variant<std::nullptr_t, int, double, std::string, bool, Array, Dict>;

    class Node : private  std::variant<std::nullptr_t, int, double, std::string, bool, Array, Dict> {
    public:
        using variant::variant;

        [[nodiscard]]bool IsInt() const;
        [[nodiscard]] bool IsDouble() const;
        [[nodiscard]] bool IsPureDouble() const;
        [[nodiscard]] bool IsBool() const;
        [[nodiscard]] bool IsString() const;
        [[nodiscard]] bool IsNull() const;
        [[nodiscard]] bool IsArray() const;
        [[nodiscard]] bool IsDict() const;
        [[nodiscard]] const NodeData& GetData() const;

        [[nodiscard]] int AsInt() const;
        [[nodiscard]] bool AsBool() const;
        [[nodiscard]] double AsDouble() const;
        [[nodiscard]] const std::string& AsString() const;
        [[nodiscard]] const Array& AsArray() const;
        [[nodiscard]] Array& AsArray();
        [[nodiscard]] const Dict& AsDict() const;
        [[nodiscard]] Dict& AsDict();

        void Clear();
    };

    class Document {
    public:
        explicit Document(Node root);
        Document() = default;

        [[nodiscard]] const Node& GetRoot() const;

        void Clear();     

    private:
        Node root_;
    };

    Document Load(std::istream& input);

    void Print(const Document& doc, std::ostream& output);


}  // namespace json