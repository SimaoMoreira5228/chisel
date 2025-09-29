#include "../../includes/tests.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

#include "toml.hpp"

TEST(ParsingNumber) {
    std::string toml_num = "number = -123.456e+2";
    try {
        toml::Value val = toml::Parser::deserialize(toml_num);
        ASSERT_TRUE(val["number"].is_number());
        ASSERT_EQ(val["number"].get_number(), -12345.6);
        std::cout << "Number parsed successfully: " << val["number"].get_number();
    } catch(const std::exception &e) {
        std::cerr << "Exception during parsing: " << e.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

TEST(ParsingString) {
    std::string toml_str = R"(text = "Hello, \"World\"!\n")";
    try {
        toml::Value val = toml::Parser::deserialize(toml_str);
        ASSERT_TRUE(val.is_object());
        ASSERT_TRUE(val["text"].is_string());
        ASSERT_EQ(val["text"].get_string(), "Hello, \"World\"!\n");
        std::cout << "String parsed successfully: " << val["text"].get_string();
    } catch(const std::exception &e) {
        std::cerr << "Exception during parsing: " << e.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

TEST(ParsingArray) {
    std::string toml_array = R"(array = [true, 123, "text", [1, 2], {key = "value"}])";
    try {
        toml::Value val = toml::Parser::deserialize(toml_array);
        ASSERT_TRUE(val["array"].is_array());
        ASSERT_EQ(val["array"].get_array().size(), 5);
        ASSERT_TRUE(val["array"][0].is_bool() && val["array"][0].get_bool() == true);
        ASSERT_EQ(val["array"][1].get_number(), 123);
        ASSERT_EQ(val["array"][2].get_string(), "text");
        ASSERT_EQ(val["array"][3].get_array().size(), 2);
        ASSERT_TRUE(val["array"][4].is_object() && val["array"][4]["key"].get_string() == "value");
        std::cout << "Array parsed successfully.";
    } catch(const std::exception &e) {
        std::cerr << "Exception during parsing: " << e.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

TEST(ParsingTable) {
    std::string toml_table =
        R"([table]
            bool_val = false
            num_val = 42
            str_val = "value"
            arr_val = [1,2]
            obj_val = {nested = true})";
    try {
        toml::Value val = toml::Parser::deserialize(toml_table);
        ASSERT_TRUE(val["table"]["bool_val"].is_bool() && val["table"]["bool_val"].get_bool() == false);
        ASSERT_EQ(val["table"]["num_val"].get_number(), 42);
        ASSERT_EQ(val["table"]["str_val"].get_string(), "value");
        ASSERT_EQ(val["table"]["arr_val"].get_array().size(), 2);
        ASSERT_TRUE(val["table"]["obj_val"].is_object() && val["table"]["obj_val"]["nested"].get_bool() == true);
        std::cout << "Table parsed successfully.";
    } catch(const std::exception &e) {
        std::cerr << "Exception during parsing: " << e.what() << std::endl;
        ASSERT_TRUE(false);
    }
}

TEST(ParsingInvalid) {
    std::string invalid_toml = R"(  {invalid toml}  )";
    bool exception_caught = false;
    try {
        toml::Value val = toml::Parser::deserialize(invalid_toml);
        std::cerr << "Expected exception for invalid TOML, but parsing succeeded." << std::endl;
    } catch(const std::exception &e) {
        exception_caught = true;
        std::cout << "Caught expected exception for invalid TOML: " << e.what();
    }
    ASSERT_TRUE(exception_caught);
}

TEST(FullParsingTest) {
    std::string complex_toml = R"(
        title = "Example"

        [owner]
        name = "John Doe"
        dob = 1979.0

        [database]
        enabled = true
        ports = [8000, 8001, 8002]
        data = [["delta", "phi"], [3.14]]
        temp_targets = { cpu = 79.5, case = 72.0 }

        [[servers]]
        host = "alpha"
        port = 8080

        [[servers]]
        host = "beta"
        port = 8081
    )";
    try {
        toml::Value val = toml::Parser::deserialize(complex_toml);
        ASSERT_EQ(val["title"].get_string(), "Example");
        ASSERT_EQ(val["owner"]["name"].get_string(), "John Doe");
        ASSERT_EQ(val["owner"]["dob"].get_number(), 1979.0);
        ASSERT_TRUE(val["database"]["enabled"].get_bool() == true);
        ASSERT_EQ(val["database"]["ports"].get_array().size(), 3);
        ASSERT_EQ(val["database"]["data"].get_array().size(), 2);
        ASSERT_EQ(val["database"]["temp_targets"]["cpu"].get_number(), 79.5);
        ASSERT_EQ(val["servers"].get_array().size(), 2);
        ASSERT_EQ(val["servers"][0]["host"].get_string(), "alpha");
        ASSERT_EQ(val["servers"][0]["port"].get_number(), 8080);
        ASSERT_EQ(val["servers"][1]["host"].get_string(), "beta");
        ASSERT_EQ(val["servers"][1]["port"].get_number(), 8081);
        std::cout << "Complex TOML parsed successfully.";
    } catch(const std::exception &e) {
        std::cerr << "Exception during parsing: " << e.what() << std::endl;
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