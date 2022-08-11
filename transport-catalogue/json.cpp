#include "json.h"

using namespace std;

namespace json {

    Document::Document(Node root) : root_(move(root)) {}

    const Node& Document::GetRoot() const {
        return root_;
    }

    Node LoadNode(istream& input) {
        static const Parser parser(input);
        return parser.Parse();
    }

    Document Load(istream& input) {
        return Document{LoadNode(input)};
    }

}