#pragma once
#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../html/html.hpp"

namespace markdown {
    enum class NodeType {
        Document,
        Heading,
        Paragraph,
        CodeBlock,
        InlineCode,
        Bold,
        Italic,
        Link,
        Image,
        List,
        ListItem,
        Quote,
        Table,
        TableRow,
        TableCell,
        Text,
        LineBreak,
        HorizontalRule
    };

    inline std::ostream &operator<<(std::ostream &os, NodeType type) {
        switch(type) {
        case NodeType::Document:
            return os << "Document";
        case NodeType::Heading:
            return os << "Heading";
        case NodeType::Paragraph:
            return os << "Paragraph";
        case NodeType::CodeBlock:
            return os << "CodeBlock";
        case NodeType::InlineCode:
            return os << "InlineCode";
        case NodeType::Bold:
            return os << "Bold";
        case NodeType::Italic:
            return os << "Italic";
        case NodeType::Link:
            return os << "Link";
        case NodeType::Image:
            return os << "Image";
        case NodeType::List:
            return os << "List";
        case NodeType::ListItem:
            return os << "ListItem";
        case NodeType::Quote:
            return os << "Quote";
        case NodeType::Table:
            return os << "Table";
        case NodeType::TableRow:
            return os << "TableRow";
        case NodeType::TableCell:
            return os << "TableCell";
        case NodeType::Text:
            return os << "Text";
        case NodeType::LineBreak:
            return os << "LineBreak";
        case NodeType::HorizontalRule:
            return os << "HorizontalRule";
        default:
            return os << "Unknown";
        }
    }

    struct Node {
        NodeType type;
        std::string text;
        std::map<std::string, std::string> attributes;
        std::vector<Node> children;
        int level = 0;

        Node() : type(NodeType::Text) {}
        explicit Node(NodeType t) : type(t) {}
        Node(NodeType t, const std::string &txt) : type(t), text(txt) {}
        Node(NodeType t, const std::string &txt, int lvl) : type(t), text(txt), level(lvl) {}
    };

    class Serializer {
    public:
        static std::string markdown(const Node &node) {
            std::ostringstream oss;
            serialize_markdown_node(oss, node, 0);
            return oss.str();
        }

        static std::string html(const Node &node) {
            html::Node html_root = convert_to_html_node(node);
            return html::Serializer::serialize(html_root);
        }

    private:
        static html::Node convert_to_html_node(const Node &md_node) {
            html::Node html_node;

            switch(md_node.type) {
            case NodeType::Document: {
                html_node.tag = "div";
                for(const auto &child : md_node.children) {
                    html_node.children.push_back(convert_to_html_node(child));
                }
                break;
            }

            case NodeType::Heading: {
                html_node.tag = "h" + std::to_string(md_node.level);
                html_node.attributes["class"] = "heading-primary";
                html_node.text = md_node.text;
                break;
            }

            case NodeType::Paragraph: {
                html_node.tag = "p";
                html_node.attributes["class"] = "paragraph";
                for(const auto &child : md_node.children) {
                    html_node.children.push_back(convert_to_html_node(child));
                }
                if(!md_node.text.empty()) {
                    html::Node text_node;
                    text_node.text = md_node.text;
                    html_node.children.push_back(text_node);
                }
                break;
            }

            case NodeType::CodeBlock: {
                html_node.tag = "pre";
                html_node.attributes["class"] = "code-block";

                html::Node code_node;
                code_node.tag = "code";
                if(md_node.attributes.count("language")) {
                    code_node.attributes["class"] = "language-" + md_node.attributes.at("language");
                }
                code_node.text = md_node.text;
                html_node.children.push_back(code_node);
                break;
            }

            case NodeType::InlineCode: {
                html_node.tag = "code";
                html_node.attributes["class"] = "inline-code";
                html_node.text = md_node.text;
                break;
            }

            case NodeType::Bold: {
                html_node.tag = "strong";
                html_node.attributes["class"] = "bold";
                for(const auto &child : md_node.children) {
                    html_node.children.push_back(convert_to_html_node(child));
                }
                if(!md_node.text.empty()) {
                    html::Node text_node;
                    text_node.text = md_node.text;
                    html_node.children.push_back(text_node);
                }
                break;
            }

            case NodeType::Italic: {
                html_node.tag = "em";
                html_node.attributes["class"] = "italic";
                for(const auto &child : md_node.children) {
                    html_node.children.push_back(convert_to_html_node(child));
                }
                if(!md_node.text.empty()) {
                    html::Node text_node;
                    text_node.text = md_node.text;
                    html_node.children.push_back(text_node);
                }
                break;
            }

            case NodeType::Link: {
                html_node.tag = "a";
                html_node.attributes["href"] = md_node.attributes.at("href");
                html_node.attributes["class"] = "link";
                html_node.text = md_node.text;
                break;
            }

            case NodeType::Image: {
                html_node.tag = "img";
                html_node.attributes["src"] = md_node.attributes.at("src");
                html_node.attributes["alt"] = md_node.attributes.at("alt");
                html_node.attributes["class"] = "image";
                break;
            }

            case NodeType::List: {
                html_node.tag = "ul";
                html_node.attributes["class"] = "list";
                for(const auto &child : md_node.children) {
                    html_node.children.push_back(convert_to_html_node(child));
                }
                break;
            }

            case NodeType::ListItem: {
                html_node.tag = "li";
                html_node.attributes["class"] = "list-item";
                for(const auto &child : md_node.children) {
                    html_node.children.push_back(convert_to_html_node(child));
                }
                if(!md_node.text.empty()) {
                    html::Node text_node;
                    text_node.text = md_node.text;
                    html_node.children.push_back(text_node);
                }
                break;
            }

            case NodeType::Quote: {
                html_node.tag = "blockquote";
                html_node.attributes["class"] = "quote";
                for(const auto &child : md_node.children) {
                    html_node.children.push_back(convert_to_html_node(child));
                }
                if(!md_node.text.empty()) {
                    html::Node text_node;
                    text_node.text = md_node.text;
                    html_node.children.push_back(text_node);
                }
                break;
            }

            case NodeType::Table: {
                html_node.tag = "table";
                html_node.attributes["class"] = "table";
                for(const auto &child : md_node.children) {
                    html_node.children.push_back(convert_to_html_node(child));
                }
                break;
            }

            case NodeType::TableRow: {
                html_node.tag = "tr";
                html_node.attributes["class"] = "table-row";
                for(const auto &child : md_node.children) {
                    html_node.children.push_back(convert_to_html_node(child));
                }
                break;
            }

            case NodeType::TableCell: {
                html_node.tag = "td";
                html_node.attributes["class"] = "table-cell";
                for(const auto &child : md_node.children) {
                    html_node.children.push_back(convert_to_html_node(child));
                }
                if(!md_node.text.empty()) {
                    html::Node text_node;
                    text_node.text = md_node.text;
                    html_node.children.push_back(text_node);
                }
                break;
            }

            case NodeType::Text: {
                html_node.text = md_node.text;
                break;
            }

            case NodeType::LineBreak: {
                html_node.tag = "br";
                html_node.attributes["class"] = "line-break";
                break;
            }

            case NodeType::HorizontalRule: {
                html_node.tag = "hr";
                html_node.attributes["class"] = "horizontal-rule";
                break;
            }
            }

            return html_node;
        }
        static void serialize_markdown_node(std::ostringstream &oss, const Node &node, int depth) {
            switch(node.type) {
            case NodeType::Document:
                for(const auto &child : node.children) {
                    serialize_markdown_node(oss, child, depth);
                    if(child.type == NodeType::Paragraph || child.type == NodeType::Heading ||
                       child.type == NodeType::CodeBlock || child.type == NodeType::List || child.type == NodeType::Quote ||
                       child.type == NodeType::Table || child.type == NodeType::HorizontalRule) {
                        oss << "\n";
                    }
                }
                break;

            case NodeType::Heading:
                oss << std::string(node.level, '#') << " " << node.text << "\n";
                break;

            case NodeType::Paragraph:
                for(const auto &child : node.children) {
                    serialize_markdown_node(oss, child, depth);
                }
                if(!node.text.empty()) {
                    oss << node.text;
                }
                oss << "\n";
                break;

            case NodeType::CodeBlock: {
                std::string language = node.attributes.count("language") ? node.attributes.at("language") : "";
                oss << "```" << language << "\n" << node.text << "\n```\n";
                break;
            }

            case NodeType::InlineCode:
                oss << "`" << node.text << "`";
                break;

            case NodeType::Bold:
                oss << "**";
                for(const auto &child : node.children) {
                    serialize_markdown_node(oss, child, depth);
                }
                if(!node.text.empty()) {
                    oss << node.text;
                }
                oss << "**";
                break;

            case NodeType::Italic:
                oss << "*";
                for(const auto &child : node.children) {
                    serialize_markdown_node(oss, child, depth);
                }
                if(!node.text.empty()) {
                    oss << node.text;
                }
                oss << "*";
                break;

            case NodeType::Link:
                oss << "[" << node.text << "](" << node.attributes.at("href") << ")";
                break;

            case NodeType::Image:
                oss << "![" << node.attributes.at("alt") << "](" << node.attributes.at("src") << ")";
                break;

            case NodeType::List:
                for(const auto &child : node.children) {
                    serialize_markdown_node(oss, child, depth);
                }
                break;

            case NodeType::ListItem: {
                std::string prefix =
                    node.attributes.count("ordered") && node.attributes.at("ordered") == "true" ? "1. " : "- ";
                oss << std::string(depth * 2, ' ') << prefix;
                for(const auto &child : node.children) {
                    serialize_markdown_node(oss, child, depth + 1);
                }
                if(!node.text.empty()) {
                    oss << node.text;
                }
                oss << "\n";
                break;
            }

            case NodeType::Quote:
                oss << "> ";
                for(const auto &child : node.children) {
                    serialize_markdown_node(oss, child, depth);
                }
                if(!node.text.empty()) {
                    oss << node.text;
                }
                oss << "\n";
                break;

            case NodeType::Table:
                for(size_t i = 0; i < node.children.size(); ++i) {
                    serialize_markdown_node(oss, node.children[i], depth);
                    if(i == 0) {
                        oss << "|";
                        for(const auto &cell : node.children[0].children) {
                            oss << " --- |";
                        }
                        oss << "\n";
                    }
                }
                break;

            case NodeType::TableRow:
                oss << "|";
                for(const auto &child : node.children) {
                    oss << " ";
                    serialize_markdown_node(oss, child, depth);
                    oss << " |";
                }
                oss << "\n";
                break;

            case NodeType::TableCell:
                if(!node.text.empty()) {
                    oss << node.text;
                }
                for(const auto &child : node.children) {
                    serialize_markdown_node(oss, child, depth);
                }
                break;

            case NodeType::Text:
                oss << node.text;
                break;

            case NodeType::LineBreak:
                oss << "  \n";
                break;

            case NodeType::HorizontalRule:
                oss << "---\n";
                break;
            }
        }
    };

    class Deserializer {
    public:
        static Node deserialize(const std::string &markdown) {
            Node document(NodeType::Document);
            std::istringstream iss(markdown);
            std::string line;
            std::vector<std::string> lines;

            while(std::getline(iss, line)) {
                lines.push_back(line);
            }

            size_t pos = 0;
            while(pos < lines.size()) {
                parse_block(lines, pos, document);
            }

            return document;
        }

    private:
        static void parse_block(const std::vector<std::string> &lines, size_t &pos, Node &parent) {
            if(pos >= lines.size())
                return;

            const std::string &line = lines[pos];

            if(line.empty()) {
                ++pos;
                return;
            }

            if(std::regex_match(line, std::regex(R"(^\s*[-*_]{3,}\s*$)"))) {
                parent.children.emplace_back(NodeType::HorizontalRule);
                ++pos;
                return;
            }

            std::smatch heading_match;
            if(std::regex_match(line, heading_match, std::regex(R"(^(#{1,6})\s+(.+)$)"))) {
                int level = heading_match[1].str().length();
                std::string text = heading_match[2].str();
                parent.children.emplace_back(NodeType::Heading, text, level);
                ++pos;
                return;
            }

            std::smatch code_match;
            if(std::regex_match(line, code_match, std::regex(R"(^```(\w*)\s*$)"))) {
                std::string language = code_match[1].str();
                ++pos;
                std::ostringstream code_content;

                while(pos < lines.size() && !std::regex_match(lines[pos], std::regex(R"(^```\s*$)"))) {
                    if(code_content.tellp() > 0)
                        code_content << "\n";
                    code_content << lines[pos];
                    ++pos;
                }

                if(pos < lines.size())
                    ++pos;

                Node code_node(NodeType::CodeBlock, code_content.str());
                if(!language.empty()) {
                    code_node.attributes["language"] = language;
                }
                parent.children.push_back(code_node);
                return;
            }

            if(std::regex_match(line, std::regex(R"(^\s*>\s*(.*)$)"))) {
                std::smatch quote_match;
                std::regex_match(line, quote_match, std::regex(R"(^\s*>\s*(.*)$)"));
                Node quote_node(NodeType::Quote);
                parse_inline(quote_match[1].str(), quote_node);
                parent.children.push_back(quote_node);
                ++pos;
                return;
            }

            std::smatch list_match;
            if(std::regex_match(line, list_match, std::regex(R"(^(\s*)[-*+]\s+(.+)$)")) ||
               std::regex_match(line, list_match, std::regex(R"(^(\s*)\d+\.\s+(.+)$)"))) {

                Node list_node(NodeType::List);
                bool is_ordered = std::regex_match(line, std::regex(R"(^(\s*)\d+\.\s+(.+)$)"));

                while(pos < lines.size()) {
                    const std::string &current_line = lines[pos];
                    std::smatch item_match;

                    if(std::regex_match(current_line, item_match, std::regex(R"(^(\s*)[-*+]\s+(.+)$)")) ||
                       std::regex_match(current_line, item_match, std::regex(R"(^(\s*)\d+\.\s+(.+)$)"))) {

                        Node item_node(NodeType::ListItem);
                        if(is_ordered) {
                            item_node.attributes["ordered"] = "true";
                        }
                        parse_inline(item_match[2].str(), item_node);
                        list_node.children.push_back(item_node);
                        ++pos;
                    } else {
                        break;
                    }
                }

                parent.children.push_back(list_node);
                return;
            }

            if(line.find('|') != std::string::npos) {
                Node table_node(NodeType::Table);

                while(pos < lines.size() && lines[pos].find('|') != std::string::npos) {
                    const std::string &table_line = lines[pos];

                    if(std::regex_match(table_line, std::regex(R"(^\s*\|[\s\-\|]*\|\s*$)"))) {
                        ++pos;
                        continue;
                    }

                    Node row_node(NodeType::TableRow);
                    std::regex cell_regex(R"(\|([^|]*))");
                    std::sregex_iterator iter(table_line.begin(), table_line.end(), cell_regex);
                    std::sregex_iterator end;

                    for(; iter != end; ++iter) {
                        std::string cell_content = iter->str(1);
                        cell_content.erase(0, cell_content.find_first_not_of(" \t"));
                        cell_content.erase(cell_content.find_last_not_of(" \t") + 1);

                        if(!cell_content.empty() ||
                           iter != std::sregex_iterator(table_line.begin(), table_line.end(), cell_regex)) {
                            Node cell_node(NodeType::TableCell);
                            parse_inline(cell_content, cell_node);
                            row_node.children.push_back(cell_node);
                        }
                    }

                    if(!row_node.children.empty()) {
                        table_node.children.push_back(row_node);
                    }
                    ++pos;
                }

                parent.children.push_back(table_node);
                return;
            }

            Node paragraph_node(NodeType::Paragraph);
            std::string paragraph_text = line;
            ++pos;

            while(pos < lines.size() && !lines[pos].empty() && !std::regex_match(lines[pos], std::regex(R"(^#{1,6}\s+)"))) {
                paragraph_text += " " + lines[pos];
                ++pos;
            }

            parse_inline(paragraph_text, paragraph_node);
            parent.children.push_back(paragraph_node);
        }

        static void parse_inline(const std::string &text, Node &parent) {
            if(text.empty())
                return;

            std::string remaining = text;
            size_t pos = 0;

            while(pos < remaining.length()) {
                std::regex bold_regex(R"(\*\*(.*?)\*\*)");
                std::smatch bold_match;
                if(std::regex_search(remaining.cbegin() + pos, remaining.cend(), bold_match, bold_regex)) {
                    size_t match_pos = pos + bold_match.prefix().length();

                    if(match_pos > pos) {
                        std::string before = remaining.substr(pos, match_pos - pos);
                        if(!before.empty()) {
                            parent.children.emplace_back(NodeType::Text, before);
                        }
                    }

                    Node bold_node(NodeType::Bold, bold_match[1].str());
                    parent.children.push_back(bold_node);

                    pos = match_pos + bold_match.length();
                    continue;
                }

                std::regex italic_regex(R"(\*(.*?)\*)");
                std::smatch italic_match;
                if(std::regex_search(remaining.cbegin() + pos, remaining.cend(), italic_match, italic_regex)) {
                    size_t match_pos = pos + italic_match.prefix().length();

                    if(match_pos > pos) {
                        std::string before = remaining.substr(pos, match_pos - pos);
                        if(!before.empty()) {
                            parent.children.emplace_back(NodeType::Text, before);
                        }
                    }

                    Node italic_node(NodeType::Italic, italic_match[1].str());
                    parent.children.push_back(italic_node);

                    pos = match_pos + italic_match.length();
                    continue;
                }

                std::regex code_regex(R"(`([^`]+)`)");
                std::smatch code_match;
                if(std::regex_search(remaining.cbegin() + pos, remaining.cend(), code_match, code_regex)) {
                    size_t match_pos = pos + code_match.prefix().length();

                    if(match_pos > pos) {
                        std::string before = remaining.substr(pos, match_pos - pos);
                        if(!before.empty()) {
                            parent.children.emplace_back(NodeType::Text, before);
                        }
                    }

                    Node code_node(NodeType::InlineCode, code_match[1].str());
                    parent.children.push_back(code_node);

                    pos = match_pos + code_match.length();
                    continue;
                }

                std::regex link_regex(R"(\[([^\]]+)\]\(([^)]+)\))");
                std::smatch link_match;
                if(std::regex_search(remaining.cbegin() + pos, remaining.cend(), link_match, link_regex)) {
                    size_t match_pos = pos + link_match.prefix().length();

                    if(match_pos > pos) {
                        std::string before = remaining.substr(pos, match_pos - pos);
                        if(!before.empty()) {
                            parent.children.emplace_back(NodeType::Text, before);
                        }
                    }

                    Node link_node(NodeType::Link, link_match[1].str());
                    link_node.attributes["href"] = link_match[2].str();
                    parent.children.push_back(link_node);

                    pos = match_pos + link_match.length();
                    continue;
                }

                std::regex img_regex(R"(!\[([^\]]*)\]\(([^)]+)\))");
                std::smatch img_match;
                if(std::regex_search(remaining.cbegin() + pos, remaining.cend(), img_match, img_regex)) {
                    size_t match_pos = pos + img_match.prefix().length();

                    if(match_pos > pos) {
                        std::string before = remaining.substr(pos, match_pos - pos);
                        if(!before.empty()) {
                            parent.children.emplace_back(NodeType::Text, before);
                        }
                    }

                    Node img_node(NodeType::Image);
                    img_node.attributes["alt"] = img_match[1].str();
                    img_node.attributes["src"] = img_match[2].str();
                    parent.children.push_back(img_node);

                    pos = match_pos + img_match.length();
                    continue;
                }

                std::string rest = remaining.substr(pos);
                if(!rest.empty()) {
                    parent.children.emplace_back(NodeType::Text, rest);
                }
                break;
            }

            if(parent.children.empty() && !text.empty()) {
                parent.children.emplace_back(NodeType::Text, text);
            }
        }
    };
} // namespace markdown