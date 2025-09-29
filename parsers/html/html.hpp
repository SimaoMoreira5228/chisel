#pragma once
#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace html {
    struct Node {
        std::string tag;
        std::string text;
        std::map<std::string, std::string> attributes;
        std::vector<Node> children;

        Node() = default;
        explicit Node(std::string t) : tag(std::move(t)) {}
        Node(std::string t, std::string txt) : tag(std::move(t)), text(std::move(txt)) {}
        Node(std::string t, std::map<std::string, std::string> attrs) : tag(std::move(t)), attributes(std::move(attrs)) {}
    };

    static std::string escape_html(const std::string &input) {
        std::string result;
        result.reserve(input.size());
        for(char c : input) {
            switch(c) {
            case '&':
                result += "&amp;";
                break;
            case '<':
                result += "&lt;";
                break;
            case '>':
                result += "&gt;";
                break;
            case '"':
                result += "&quot;";
                break;
            case '\'':
                result += "&#39;";
                break;
            default:
                result += c;
                break;
            }
        }
        return result;
    }

    static std::string unescape_html(const std::string &input) {
        std::string result;
        result.reserve(input.size());
        for(size_t i = 0; i < input.size(); ++i) {
            if(input[i] == '&') {
                if(input.substr(i, 5) == "&amp;") {
                    result += '&';
                    i += 4;
                } else if(input.substr(i, 4) == "&lt;") {
                    result += '<';
                    i += 3;
                } else if(input.substr(i, 4) == "&gt;") {
                    result += '>';
                    i += 3;
                } else if(input.substr(i, 6) == "&quot;") {
                    result += '"';
                    i += 5;
                } else if(input.substr(i, 5) == "&#39;") {
                    result += '\'';
                    i += 4;
                } else {
                    result += input[i];
                }
            } else {
                result += input[i];
            }
        }
        return result;
    }

    class Serializer {
    public:
        static std::string serialize(const Node &node, int indent_level = 0) {
            std::ostringstream oss;
            const std::string indent(indent_level * 2, ' ');

            if(!node.tag.empty()) {
                static const std::set<std::string> self_closing_tags = {"img", "hr", "br"};
                static const std::set<std::string> inline_tags = {"strong", "em", "a", "code", "span"};
                oss << indent << "<" << node.tag;
                for(const auto &[key, value] : node.attributes) {
                    oss << " " << key << "=\"" << html::escape_html(value) << "\"";
                }

                if(self_closing_tags.count(node.tag)) {
                    oss << " />";
                    return oss.str();
                }

                oss << ">";

                if(!node.text.empty()) {
                    auto class_attr = node.attributes.find("class");
                    if(node.tag == "code" && class_attr != node.attributes.end() && !class_attr->second.empty() &&
                       class_attr->second.find("language-") == 0) {
                        oss << node.text;
                    } else {
                        oss << html::escape_html(node.text);
                    }
                }

                bool is_inline = inline_tags.count(node.tag) > 0;
                for(const auto &child : node.children) {
                    if(!is_inline) {
                        oss << "\n";
                    }
                    oss << serialize(child, is_inline ? 0 : indent_level + 1);
                }

                if(!node.children.empty() && !is_inline) {
                    oss << "\n" << indent;
                }
                oss << "</" << node.tag << ">";
            } else if(!node.text.empty()) {
                oss << indent << html::escape_html(node.text);
            }

            return oss.str();
        }
    };

    class Deserializer {
    public:
        static Node deserialize(const std::string &html) {
            size_t pos = 0;
            Node result = parse_node(html, pos);
            if(result.tag.empty()) {
                throw std::invalid_argument("Invalid HTML: No root element found");
            }
            return result;
        }

    private:
        static void skip_whitespace(const std::string &html, size_t &pos) {
            while(pos < html.length() && std::isspace(html[pos])) {
                ++pos;
            }
        }

        static Node parse_node(const std::string &html, size_t &pos) {
            Node node;
            skip_whitespace(html, pos);

            if(pos >= html.length() || html[pos] != '<') {
                return node;
            }

            ++pos;
            if(pos >= html.length()) {
                throw std::invalid_argument("Invalid HTML: Unexpected end of input");
            }

            if(html[pos] == '/') {
                return node;
            }

            std::string tag;
            while(pos < html.length() && html[pos] != ' ' && html[pos] != '>' && html[pos] != '/') {
                tag += std::tolower(html[pos++]);
            }

            if(tag.empty()) {
                throw std::invalid_argument("Invalid HTML: Empty tag name");
            }

            node.tag = tag;

            skip_whitespace(html, pos);
            while(pos < html.length() && html[pos] != '>' && html[pos] != '/') {
                std::string key, value;

                while(pos < html.length() && html[pos] != '=' && html[pos] != '>' && html[pos] != '/' &&
                      !std::isspace(html[pos])) {
                    key += std::tolower(html[pos++]);
                }

                skip_whitespace(html, pos);

                if(pos < html.length() && html[pos] == '=') {
                    ++pos;
                    skip_whitespace(html, pos);

                    if(pos < html.length() && html[pos] == '"') {
                        ++pos;
                        while(pos < html.length() && html[pos] != '"') {
                            value += html[pos++];
                        }
                        if(pos < html.length()) {
                            ++pos;
                        }
                    } else {
                        while(pos < html.length() && html[pos] != ' ' && html[pos] != '>' && html[pos] != '/') {
                            value += html[pos++];
                        }
                    }
                }

                if(!key.empty()) {
                    node.attributes[key] = value;
                }

                skip_whitespace(html, pos);
            }

            if(pos < html.length() && html[pos] == '/') {
                ++pos;
                if(pos < html.length() && html[pos] == '>') {
                    ++pos;
                }
                return node;
            }

            if(pos < html.length() && html[pos] == '>') {
                ++pos;
            } else {
                throw std::invalid_argument("Invalid HTML: Expected '>' after tag");
            }

            std::string text;
            while(pos < html.length()) {
                if(html[pos] == '<') {
                    if(pos + 1 < html.length() && html[pos + 1] == '/') {
                        break;
                    } else {
                        if(!text.empty()) {
                            node.text = unescape_html(text);
                            text.clear();
                        }
                        Node child = parse_node(html, pos);
                        if(!child.tag.empty()) {
                            node.children.push_back(child);
                        }
                    }
                } else {
                    text += html[pos++];
                }
            }

            if(!text.empty()) {
                std::string trimmed_text = text;
                size_t start = trimmed_text.find_first_not_of(" \t\n\r");
                if(start != std::string::npos) {
                    size_t end = trimmed_text.find_last_not_of(" \t\n\r");
                    trimmed_text = trimmed_text.substr(start, end - start + 1);
                } else {
                    trimmed_text.clear();
                }
                node.text = unescape_html(trimmed_text);
            }

            if(pos < html.length() && html[pos] == '<' && pos + 1 < html.length() && html[pos + 1] == '/') {
                pos += 2;
                std::string end_tag;
                while(pos < html.length() && html[pos] != '>') {
                    end_tag += std::tolower(html[pos++]);
                }
                if(end_tag != tag) {
                    throw std::invalid_argument("Invalid HTML: Mismatched closing tag");
                }
                if(pos < html.length()) {
                    ++pos;
                } else {
                    throw std::invalid_argument("Invalid HTML: Unclosed tag");
                }
            } else {
                throw std::invalid_argument("Invalid HTML: Missing closing tag for " + tag);
            }

            return node;
        }
    };
} // namespace html