#include "../../includes/tests.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

#include "json.hpp"

TEST(ParsingNumber) {
    std::string json_num = "  -123.456e+2  ";
    try {
        json::Value val = json::Parser::deserialize(json_num);
        ASSERT_TRUE(val.is_number());
        ASSERT_EQ(val.get_number(), -12345.6);
        std::cout << "Number parsed successfully: " << val.get_number();
    } catch(const std::exception &e) {
        std::cerr << "Exception during parsing: " << e.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

TEST(ParsingString) {
    std::string json_str = R"(  "Hello, \"World\"!\n"  )";
    try {
        json::Value val = json::Parser::deserialize(json_str);
        ASSERT_TRUE(val.is_string());
        ASSERT_EQ(val.get_string(), "Hello, \"World\"!\n");
        std::cout << "String parsed successfully: " << val.get_string();
    } catch(const std::exception &e) {
        std::cerr << "Exception during parsing: " << e.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

TEST(ParsingArray) {
    std::string json_array = R"(  [null, true, 123, "text", [1, 2], {"key": "value"}]  )";
    try {
        json::Value val = json::Parser::deserialize(json_array);
        ASSERT_TRUE(val[0].is_null());
        ASSERT_TRUE(val[1].is_bool() && val[1].get_bool() == true);
        ASSERT_EQ(val[2].get_number(), 123);
        ASSERT_EQ(val[3].get_string(), "text");
        ASSERT_EQ(val[4].get_array().size(), 2);
        ASSERT_TRUE(val[5].is_object() && val[5]["key"].get_string() == "value");
        std::cout << "Array parsed successfully.";
    } catch(const std::exception &e) {
        std::cerr << "Exception during parsing: " << e.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

TEST(ParsingObject) {
    std::string json_obj =
        R"(  {"null": null, "bool": false, "num": 42, "str": "value", "arr": [1,2], "obj": {"nested": true}}  )";
    try {
        json::Value val = json::Parser::deserialize(json_obj);
        ASSERT_TRUE(val["null"].is_null());
        ASSERT_TRUE(val["bool"].is_bool() && val["bool"].get_bool() == false);
        ASSERT_EQ(val["num"].get_number(), 42);
        ASSERT_EQ(val["str"].get_string(), "value");
        ASSERT_EQ(val["arr"].get_array().size(), 2);
        ASSERT_TRUE(val["obj"].is_object() && val["obj"]["nested"].get_bool() == true);
        std::cout << "Object parsed successfully.";
    } catch(const std::exception &e) {
        std::cerr << "Exception during parsing: " << e.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

TEST(ParsingInvalid) {
    std::string invalid_json = R"(  {invalid json}  )";
    bool exception_caught = false;
    try {
        json::Value val = json::Parser::deserialize(invalid_json);
        std::cerr << "Expected exception for invalid JSON, but parsing succeeded." << std::endl;
    } catch(const std::exception &e) {
        exception_caught = true;
        std::cout << "Caught expected exception for invalid JSON: " << e.what();
    }
    ASSERT_TRUE(exception_caught);
}

TEST(FullParsingTest) {
    std::string complex_json = R"({
        "name": "Test",
        "age": 30,
        "is_student": false,
        "scores": [95.5, 88.0, 76.5],
        "address": {
            "street": "123 Main St",
            "city": "Anytown"
        },
        "null_value": null
    })";
    try {
        json::Value val = json::Parser::deserialize(complex_json);
        ASSERT_EQ(val["name"].get_string(), "Test");
        ASSERT_EQ(val["age"].get_number(), 30);
        ASSERT_TRUE(val["is_student"].get_bool() == false);
        ASSERT_EQ(val["scores"].get_array().size(), 3);
        ASSERT_EQ(val["address"]["city"].get_string(), "Anytown");
        ASSERT_TRUE(val["null_value"].is_null());
        std::cout << "Complex JSON parsed successfully.";
    } catch(const std::exception &e) {
        std::cerr << "Exception during parsing complex JSON: " << e.what() << std::endl;
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