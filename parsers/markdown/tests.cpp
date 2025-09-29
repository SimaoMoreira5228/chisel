#include "../../includes/tests.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

#include "markdown.hpp"

std::string normalize_html(const std::string &html) {
    std::string result;
    result.reserve(html.size());
    bool in_whitespace = false;
    for(char c : html) {
        if(std::isspace(c)) {
            if(!in_whitespace) {
                result += ' ';
                in_whitespace = true;
            }
        } else {
            result += c;
            in_whitespace = false;
        }
    }

    size_t start = result.find_first_not_of(' ');
    size_t end = result.find_last_not_of(' ');
    return start == std::string::npos ? "" : result.substr(start, end - start + 1);
}

TEST(ParsingSimpleText) {
    std::string markdown_input = "Hello World";
    try {
        markdown::Node node = markdown::Deserializer::deserialize(markdown_input);
        ASSERT_EQ(node.type, markdown::NodeType::Document);
        ASSERT_EQ(node.children.size(), 1);
        ASSERT_EQ(node.children[0].type, markdown::NodeType::Paragraph);
        ASSERT_EQ(node.children[0].children.size(), 1);
        ASSERT_EQ(node.children[0].children[0].text, "Hello World");
        std::cout << "Simple text parsed successfully.";
    } catch(const std::exception &e) {
        std::cerr << "Exception during parsing: " << e.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

TEST(ParsingHeading) {
    std::string markdown_input = "# Main Title\n## Subtitle";
    try {
        markdown::Node node = markdown::Deserializer::deserialize(markdown_input);
        ASSERT_EQ(node.type, markdown::NodeType::Document);
        ASSERT_EQ(node.children.size(), 2);

        ASSERT_EQ(node.children[0].type, markdown::NodeType::Heading);
        ASSERT_EQ(node.children[0].level, 1);
        ASSERT_EQ(node.children[0].text, "Main Title");

        ASSERT_EQ(node.children[1].type, markdown::NodeType::Heading);
        ASSERT_EQ(node.children[1].level, 2);
        ASSERT_EQ(node.children[1].text, "Subtitle");

        std::cout << "Headings parsed successfully: H" << node.children[0].level << " and H" << node.children[1].level;
    } catch(const std::exception &e) {
        std::cerr << "Exception during parsing: " << e.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

TEST(ParsingCodeBlock) {
    std::string markdown_input = "```cpp\nint main() {\n    return 0;\n}\n```";
    try {
        markdown::Node node = markdown::Deserializer::deserialize(markdown_input);
        ASSERT_EQ(node.type, markdown::NodeType::Document);
        ASSERT_EQ(node.children.size(), 1);
        ASSERT_EQ(node.children[0].type, markdown::NodeType::CodeBlock);
        ASSERT_EQ(node.children[0].attributes["language"], "cpp");
        ASSERT_EQ(node.children[0].text, "int main() {\n    return 0;\n}");
        std::cout << "Code block parsed successfully with language: " << node.children[0].attributes["language"];
    } catch(const std::exception &e) {
        std::cerr << "Exception during parsing: " << e.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

TEST(ParsingInlineFormatting) {
    std::string markdown_input = "This is **bold** and *italic* and `code`.";
    try {
        markdown::Node node = markdown::Deserializer::deserialize(markdown_input);
        ASSERT_EQ(node.type, markdown::NodeType::Document);
        ASSERT_EQ(node.children.size(), 1);
        ASSERT_EQ(node.children[0].type, markdown::NodeType::Paragraph);

        ASSERT_TRUE(node.children[0].children.size() >= 3);

        bool found_bold = false, found_italic = false, found_code = false;
        for(const auto &child : node.children[0].children) {
            if(child.type == markdown::NodeType::Bold && child.text == "bold") {
                found_bold = true;
            } else if(child.type == markdown::NodeType::Italic && child.text == "italic") {
                found_italic = true;
            } else if(child.type == markdown::NodeType::InlineCode && child.text == "code") {
                found_code = true;
            }
        }

        ASSERT_TRUE(found_bold);
        ASSERT_TRUE(found_italic);
        ASSERT_TRUE(found_code);

        std::cout << "Inline formatting parsed successfully: bold, italic, and code.";
    } catch(const std::exception &e) {
        std::cerr << "Exception during parsing: " << e.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

TEST(ParsingLinks) {
    std::string markdown_input = "Check out [GitHub](https://github.com) for more info.";
    try {
        markdown::Node node = markdown::Deserializer::deserialize(markdown_input);
        ASSERT_EQ(node.type, markdown::NodeType::Document);
        ASSERT_EQ(node.children.size(), 1);
        ASSERT_EQ(node.children[0].type, markdown::NodeType::Paragraph);

        bool found_link = false;
        for(const auto &child : node.children[0].children) {
            if(child.type == markdown::NodeType::Link && child.text == "GitHub" &&
               child.attributes.at("href") == "https://github.com") {
                found_link = true;
                break;
            }
        }

        ASSERT_TRUE(found_link);
        std::cout << "Link parsed successfully: GitHub -> https://github.com";
    } catch(const std::exception &e) {
        std::cerr << "Exception during parsing: " << e.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

TEST(ParsingLists) {
    std::string markdown_input = "- Item 1\n- Item 2\n- Item 3";
    try {
        markdown::Node node = markdown::Deserializer::deserialize(markdown_input);
        ASSERT_EQ(node.type, markdown::NodeType::Document);
        ASSERT_EQ(node.children.size(), 1);
        ASSERT_EQ(node.children[0].type, markdown::NodeType::List);
        ASSERT_EQ(node.children[0].children.size(), 3);

        for(int i = 0; i < 3; ++i) {
            ASSERT_EQ(node.children[0].children[i].type, markdown::NodeType::ListItem);
            std::string expected = "Item " + std::to_string(i + 1);
            ASSERT_EQ(node.children[0].children[i].children[0].text, expected);
        }

        std::cout << "Unordered list parsed successfully with 3 items.";
    } catch(const std::exception &e) {
        std::cerr << "Exception during parsing: " << e.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

TEST(MarkdownSerialization) {
    markdown::Node document(markdown::NodeType::Document);

    document.children.emplace_back(markdown::NodeType::Heading, "Test Document", 1);

    markdown::Node paragraph(markdown::NodeType::Paragraph);
    paragraph.children.emplace_back(markdown::NodeType::Text, "This is ");
    paragraph.children.emplace_back(markdown::NodeType::Bold, "bold text");
    paragraph.children.emplace_back(markdown::NodeType::Text, " and ");
    paragraph.children.emplace_back(markdown::NodeType::InlineCode, "inline code");
    paragraph.children.emplace_back(markdown::NodeType::Text, ".");
    document.children.push_back(paragraph);

    markdown::Node code_block(markdown::NodeType::CodeBlock, "console.log('Hello, World!');");
    code_block.attributes["language"] = "javascript";
    document.children.push_back(code_block);

    try {
        std::string serialized = markdown::Serializer::markdown(document);
        std::cout << "Markdown serialized successfully:\n" << serialized;

        ASSERT_TRUE(serialized.find("# Test Document") != std::string::npos);
        ASSERT_TRUE(serialized.find("**bold text**") != std::string::npos);
        ASSERT_TRUE(serialized.find("`inline code`") != std::string::npos);
        ASSERT_TRUE(serialized.find("```javascript") != std::string::npos);

    } catch(const std::exception &e) {
        std::cerr << "Exception during markdown serialization: " << e.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

TEST(HtmlSerialization) {
    markdown::Node document(markdown::NodeType::Document);

    document.children.emplace_back(markdown::NodeType::Heading, "Test Document", 1);

    markdown::Node paragraph(markdown::NodeType::Paragraph);
    paragraph.children.emplace_back(markdown::NodeType::Text, "This is ");
    paragraph.children.emplace_back(markdown::NodeType::Bold, "bold text");
    paragraph.children.emplace_back(markdown::NodeType::Text, " and ");
    paragraph.children.emplace_back(markdown::NodeType::InlineCode, "inline code");
    paragraph.children.emplace_back(markdown::NodeType::Text, ".");
    document.children.push_back(paragraph);

    markdown::Node code_block(markdown::NodeType::CodeBlock, "console.log('Hello, World!');");
    code_block.attributes["language"] = "javascript";
    document.children.push_back(code_block);

    try {
        std::string html_output = markdown::Serializer::html(document);
        std::string normalized_html = normalize_html(html_output);
        std::cout << "HTML serialized successfully:\n" << html_output;

        ASSERT_TRUE(normalized_html.find(normalize_html("<h1 class=\"heading-primary\">Test Document</h1>")) !=
                    std::string::npos);
        ASSERT_TRUE(normalized_html.find(normalize_html("<strong class=\"bold\">bold text</strong>")) != std::string::npos);
        ASSERT_TRUE(normalized_html.find(normalize_html("<code class=\"inline-code\">inline code</code>")) !=
                    std::string::npos);
        ASSERT_TRUE(normalized_html.find(
                        normalize_html("<pre class=\"code-block\"> <code class=\"language-javascript\">console.log('Hello, "
                                       "World!');</code> </pre>")) != std::string::npos);
        ASSERT_TRUE(normalized_html.find(normalize_html("<p class=\"paragraph\">")) != std::string::npos);

    } catch(const std::exception &e) {
        std::cerr << "Exception during HTML serialization: " << e.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

TEST(HtmlEscaping) {
    markdown::Node document(markdown::NodeType::Document);

    document.children.emplace_back(markdown::NodeType::Heading, "Title with <script> & \"quotes\"", 1);

    markdown::Node paragraph(markdown::NodeType::Paragraph);
    paragraph.children.emplace_back(markdown::NodeType::Text, "Text with <tags> & 'quotes'");
    document.children.push_back(paragraph);

    markdown::Node code_block(markdown::NodeType::CodeBlock, "if (x < y && z > 0) { print(\"Hello\"); }");
    document.children.push_back(code_block);

    markdown::Node link(markdown::NodeType::Link, "Link & Text");
    link.attributes["href"] = "https://example.com?param=<value>&other=\"test\"";
    document.children.push_back(link);

    try {
        std::string html_output = markdown::Serializer::html(document);
        std::cout << "HTML with escaping:\n" << html_output;

        ASSERT_TRUE(html_output.find("&lt;script&gt;") != std::string::npos);
        ASSERT_TRUE(html_output.find("&amp;") != std::string::npos);
        ASSERT_TRUE(html_output.find("&quot;") != std::string::npos);
        ASSERT_TRUE(html_output.find("&#39;") != std::string::npos);
        ASSERT_TRUE(html_output.find("&lt;tags&gt;") != std::string::npos);

    } catch(const std::exception &e) {
        std::cerr << "Exception during HTML escaping test: " << e.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

TEST(HtmlLinks) {
    markdown::Node document(markdown::NodeType::Document);

    markdown::Node link(markdown::NodeType::Link, "GitHub");
    link.attributes["href"] = "https://github.com";
    document.children.push_back(link);

    markdown::Node image(markdown::NodeType::Image);
    image.attributes["src"] = "https://example.com/image.png";
    image.attributes["alt"] = "Example Image";
    document.children.push_back(image);

    try {
        std::string html_output = markdown::Serializer::html(document);
        std::cout << "HTML links and images:\n" << html_output;

        ASSERT_TRUE(html_output.find("<a class=\"link\" href=\"https://github.com\">GitHub</a>") != std::string::npos);
        ASSERT_TRUE(
            html_output.find("<img alt=\"Example Image\" class=\"image\" src=\"https://example.com/image.png\" />") !=
            std::string::npos);

    } catch(const std::exception &e) {
        std::cerr << "Exception during HTML links test: " << e.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

TEST(HtmlLists) {
    markdown::Node document(markdown::NodeType::Document);

    markdown::Node list(markdown::NodeType::List);

    markdown::Node item1(markdown::NodeType::ListItem, "First item");
    markdown::Node item2(markdown::NodeType::ListItem, "Second item");
    markdown::Node item3(markdown::NodeType::ListItem);
    item3.children.emplace_back(markdown::NodeType::Text, "Third item with ");
    item3.children.emplace_back(markdown::NodeType::Bold, "bold text");

    list.children.push_back(item1);
    list.children.push_back(item2);
    list.children.push_back(item3);

    document.children.push_back(list);

    try {
        std::string html_output = markdown::Serializer::html(document);
        std::cout << "HTML list:\n" << html_output;

        ASSERT_TRUE(html_output.find("<ul class=\"list\">") != std::string::npos);
        ASSERT_TRUE(html_output.find("<li class=\"list-item\">\n      First item\n    </li>") != std::string::npos);
        ASSERT_TRUE(html_output.find("<li class=\"list-item\">\n      Second item\n    </li>") != std::string::npos);
        ASSERT_TRUE(html_output.find("<strong class=\"bold\">bold text</strong>") != std::string::npos);
        ASSERT_TRUE(html_output.find("</ul>") != std::string::npos);

    } catch(const std::exception &e) {
        std::cerr << "Exception during HTML lists test: " << e.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

TEST(HtmlTable) {
    markdown::Node document(markdown::NodeType::Document);

    markdown::Node table(markdown::NodeType::Table);

    markdown::Node header_row(markdown::NodeType::TableRow);
    header_row.children.emplace_back(markdown::NodeType::TableCell, "Name");
    header_row.children.emplace_back(markdown::NodeType::TableCell, "Age");
    header_row.children.emplace_back(markdown::NodeType::TableCell, "City");
    table.children.push_back(header_row);

    markdown::Node data_row(markdown::NodeType::TableRow);
    data_row.children.emplace_back(markdown::NodeType::TableCell, "John");
    data_row.children.emplace_back(markdown::NodeType::TableCell, "25");
    markdown::Node city_cell(markdown::NodeType::TableCell);
    city_cell.children.emplace_back(markdown::NodeType::Text, "New ");
    city_cell.children.emplace_back(markdown::NodeType::Bold, "York");
    data_row.children.push_back(city_cell);
    table.children.push_back(data_row);

    document.children.push_back(table);

    try {
        std::string html_output = markdown::Serializer::html(document);
        std::cout << "HTML table:\n" << html_output;

        ASSERT_TRUE(html_output.find("<table class=\"table\">") != std::string::npos);
        ASSERT_TRUE(html_output.find("<tr class=\"table-row\">") != std::string::npos);
        ASSERT_TRUE(html_output.find("<td class=\"table-cell\">\n        Name\n      </td>") != std::string::npos);
        ASSERT_TRUE(html_output.find("<td class=\"table-cell\">\n        John\n      </td>") != std::string::npos);
        ASSERT_TRUE(html_output.find("<strong class=\"bold\">York</strong>") != std::string::npos);
        ASSERT_TRUE(html_output.find("</table>") != std::string::npos);

    } catch(const std::exception &e) {
        std::cerr << "Exception during HTML table test: " << e.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

TEST(HtmlQuoteAndHorizontalRule) {
    markdown::Node document(markdown::NodeType::Document);

    markdown::Node quote(markdown::NodeType::Quote);
    quote.children.emplace_back(markdown::NodeType::Text, "This is a ");
    quote.children.emplace_back(markdown::NodeType::Italic, "quoted");
    quote.children.emplace_back(markdown::NodeType::Text, " text.");
    document.children.push_back(quote);

    document.children.emplace_back(markdown::NodeType::HorizontalRule);

    try {
        std::string html_output = markdown::Serializer::html(document);
        std::cout << "HTML quote and HR:\n" << html_output;

        ASSERT_TRUE(html_output.find("<blockquote class=\"quote\">") != std::string::npos);
        ASSERT_TRUE(html_output.find("<em class=\"italic\">quoted</em>") != std::string::npos);
        ASSERT_TRUE(html_output.find("</blockquote>") != std::string::npos);
        ASSERT_TRUE(html_output.find("<hr class=\"horizontal-rule\" />") != std::string::npos);

    } catch(const std::exception &e) {
        std::cerr << "Exception during HTML quote/HR test: " << e.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

TEST(MarkdownToHtmlRoundtrip) {
    std::string markdown_input = R"(# Main Title

This is a paragraph with **bold**, *italic*, and `inline code`.

## Code Example

```python
def hello():
    print("Hello, World!")
```

### Links and Lists

Check out [GitHub](https://github.com) for more info.

- Item 1
- Item 2 with **bold text**
- Item 3

---

> This is a blockquote with *emphasized* text.

| Name | Age | City |
|------|-----|------|
| John | 25  | NYC  |
| Jane | 30  | LA   |)";

    try {
        markdown::Node document = markdown::Deserializer::deserialize(markdown_input);

        std::string html_output = markdown::Serializer::html(document);

        std::string markdown_output = markdown::Serializer::markdown(document);

        std::cout << "Full round-trip test completed.\n";
        std::cout << "HTML output length: " << html_output.length() << "\n";
        std::cout << "Markdown output length: " << markdown_output.length() << "\n";

        ASSERT_TRUE(html_output.find("<h1 class=\"heading-primary\">Main Title</h1>") != std::string::npos);
        ASSERT_TRUE(html_output.find("<h2 class=\"heading-primary\">Code Example</h2>") != std::string::npos);
        ASSERT_TRUE(html_output.find("<strong class=\"bold\">bold</strong>") != std::string::npos);
        ASSERT_TRUE(html_output.find("<em class=\"italic\">italic</em>") != std::string::npos);
        ASSERT_TRUE(html_output.find("<code class=\"inline-code\">inline code</code>") != std::string::npos);
        ASSERT_TRUE(html_output.find("class=\"language-python\"") != std::string::npos);
        ASSERT_TRUE(html_output.find("<a class=\"link\" href=\"https://github.com\">GitHub</a>") != std::string::npos);
        ASSERT_TRUE(html_output.find("<ul class=\"list\">") != std::string::npos);
        ASSERT_TRUE(html_output.find("<blockquote class=\"quote\">") != std::string::npos);
        ASSERT_TRUE(html_output.find("<table class=\"table\">") != std::string::npos);
        ASSERT_TRUE(html_output.find("<hr class=\"horizontal-rule\" />") != std::string::npos);

        ASSERT_TRUE(markdown_output.find("# Main Title") != std::string::npos);
        ASSERT_TRUE(markdown_output.find("**bold**") != std::string::npos);
        ASSERT_TRUE(markdown_output.find("*italic*") != std::string::npos);
        ASSERT_TRUE(markdown_output.find("`inline code`") != std::string::npos);
        ASSERT_TRUE(markdown_output.find("```python") != std::string::npos);
        ASSERT_TRUE(markdown_output.find("[GitHub](https://github.com)") != std::string::npos);

    } catch(const std::exception &e) {
        std::cerr << "Exception during round-trip test: " << e.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

TEST(ParseAndSerializeRoundtrip) {
    std::string original_markdown = R"(# GitHub Markdown Test

This is a **bold** statement with *italic* text and `inline code`.

## Code Example

```python
def hello_world():
    print("Hello, World!")
```

## Links and Images

Check out [GitHub](https://github.com) for more info.

## Lists

- First item
- Second item
- Third item

---

That's all!)";

    try {
        markdown::Node document = markdown::Deserializer::deserialize(original_markdown);

        std::string serialized = markdown::Serializer::markdown(document);

        std::cout << "Round-trip test completed successfully.\n";
        std::cout << "Original length: " << original_markdown.length() << "\n";
        std::cout << "Serialized length: " << serialized.length();

        ASSERT_TRUE(serialized.find("# GitHub Markdown Test") != std::string::npos);
        ASSERT_TRUE(serialized.find("**bold**") != std::string::npos);
        ASSERT_TRUE(serialized.find("*italic*") != std::string::npos);
        ASSERT_TRUE(serialized.find("`inline code`") != std::string::npos);
        ASSERT_TRUE(serialized.find("```python") != std::string::npos);
        ASSERT_TRUE(serialized.find("[GitHub](https://github.com)") != std::string::npos);
        ASSERT_TRUE(serialized.find("- First item") != std::string::npos);
        ASSERT_TRUE(serialized.find("---") != std::string::npos);

    } catch(const std::exception &e) {
        std::cerr << "Exception during round-trip test: " << e.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

#ifdef ENABLE_TESTS
int main() {
    return Test::RunAllTests();
    return 0;
}
#else
int main() { return 0; }
#endif