#include "../../includes/tests.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

#include "html.hpp"

TEST(ParsingSimpleTag) {
    std::string html_input = "<div>Hello World</div>";
    try {
        html::Node node = html::Deserializer::deserialize(html_input);
        ASSERT_EQ(node.tag, "div");
        ASSERT_EQ(node.text, "Hello World");
        std::cout << "Simple tag parsed successfully: <" << node.tag << "> with text: " << node.text;
    } catch(const std::exception &e) {
        std::cerr << "Exception during parsing: " << e.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

TEST(ParsingNestedTags) {
    std::string html_input = "<div><p>Paragraph 1</p><p>Paragraph 2</p></div>";
    try {
        html::Node node = html::Deserializer::deserialize(html_input);
        ASSERT_EQ(node.tag, "div");
        ASSERT_EQ(node.children.size(), 2);
        ASSERT_EQ(node.children[0].tag, "p");
        ASSERT_EQ(node.children[0].text, "Paragraph 1");
        ASSERT_EQ(node.children[1].tag, "p");
        ASSERT_EQ(node.children[1].text, "Paragraph 2");
        std::cout << "Nested tags parsed successfully: <" << node.tag << "> with " << node.children.size() << " children."
                 ;
    } catch(const std::exception &e) {
        std::cerr << "Exception during parsing: " << e.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

TEST(ParsingAttributes) {
    std::string html_input = R"(<a href="https://example.com" title="Example">Link</a>)";
    try {
        html::Node node = html::Deserializer::deserialize(html_input);
        ASSERT_EQ(node.tag, "a");
        ASSERT_EQ(node.text, "Link");
        ASSERT_EQ(node.attributes["href"], "https://example.com");
        ASSERT_EQ(node.attributes["title"], "Example");
        std::cout << "Tag with attributes parsed successfully: <" << node.tag << "> with text: " << node.text
                  << " and attributes.";
    } catch(const std::exception &e) {
        std::cerr << "Exception during parsing: " << e.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

TEST(Serialization) {
    html::Node node;
    node.tag = "div";
    node.children.push_back(html::Node());
    node.children[0].tag = "p";
    node.children[0].text = "Hello World";
    node.children.push_back(html::Node());
    node.children[1].tag = "a";
    node.children[1].text = "Link";
    node.children[1].attributes["href"] = "https://example.com";

    try {
        std::string serialized_html = html::Serializer::serialize(node);
        std::string expected_html = R"(<div>
  <p>Hello World</p>
  <a href="https://example.com">Link</a>
</div>)";
        ASSERT_EQ(serialized_html, expected_html);
        std::cout << "HTML serialized successfully:\n" << serialized_html;
    } catch(const std::exception &e) {
        std::cerr << "Exception during serialization: " << e.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

TEST(ParsingWithEscapedCharacters) {
    std::string html_input = R"(<p>This &amp; that &lt; those &gt; these</p>)";
    try {
        html::Node node = html::Deserializer::deserialize(html_input);
        ASSERT_EQ(node.tag, "p");
        ASSERT_EQ(node.text, "This & that < those > these");
        std::cout << "Tag with escaped characters parsed successfully: <" << node.tag << "> with text: " << node.text
                 ;
    } catch(const std::exception &e) {
        std::cerr << "Exception during parsing: " << e.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

TEST(SerializationWithEscapedCharacters) {
    html::Node node;
    node.tag = "p";
    node.text = R"(This & that < those > these "quotes" 'single quotes')";

    try {
        std::string serialized_html = html::Serializer::serialize(node);
        std::string expected_html =
            R"(<p>This &amp; that &lt; those &gt; these &quot;quotes&quot; &#39;single quotes&#39;</p>)";
        ASSERT_EQ(serialized_html, expected_html);
        std::cout << "HTML with escaped characters serialized successfully:\n" << serialized_html;
    } catch(const std::exception &e) {
        std::cerr << "Exception during serialization: " << e.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

TEST(ParsingSelfClosingTag) {
    std::string html_input = R"(<img src="image.png" alt="An image" />)";
    try {
        html::Node node = html::Deserializer::deserialize(html_input);
        ASSERT_EQ(node.tag, "img");
        ASSERT_EQ(node.attributes["src"], "image.png");
        ASSERT_EQ(node.attributes["alt"], "An image");
        ASSERT_TRUE(node.children.empty());
        ASSERT_TRUE(node.text.empty());
        std::cout << "Self-closing tag parsed successfully: <" << node.tag << "> with attributes.";
    } catch(const std::exception &e) {
        std::cerr << "Exception during parsing: " << e.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

TEST(ParsingInvalidHTML) {
    std::string invalid_html = "<div><p>Unclosed div";
    bool exception_caught = false;
    try {
        html::Node node = html::Deserializer::deserialize(invalid_html);
        std::cerr << "Expected exception for invalid HTML, but parsing succeeded." << std::endl;
    } catch(const std::exception &e) {
        exception_caught = true;
        std::cout << "Caught expected exception for invalid HTML: " << e.what();
    }
    ASSERT_TRUE(exception_caught);
}

#ifdef ENABLE_TESTS
int main() {
    return Test::RunAllTests();
    return 0;
}
#else
int main() { return 0; }
#endif