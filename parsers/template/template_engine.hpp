#pragma once

#include <chrono>
#include <functional>
#include <iomanip>
#include <map>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace ssg::template_engine {
    class TemplateEngine;
    struct TemplateValue;

    using TemplateHelper = std::function<std::string(const std::vector<TemplateValue> &)>;

    struct TemplateError {
        enum Type { SYNTAX_ERROR, VARIABLE_NOT_FOUND, HELPER_ERROR, PARSE_ERROR };
        Type type;
        std::string message;
        size_t position;

        TemplateError(Type t, const std::string &msg, size_t pos = 0) : type(t), message(msg), position(pos) {}
    };

    struct TemplateValue {
        enum Type { STRING, ARRAY, BOOLEAN, OBJECT, NUMBER, DATE };
        Type type;
        std::string string_value;
        std::vector<TemplateValue> array_value;
        bool boolean_value;
        std::map<std::string, TemplateValue> object_value;
        double number_value;
        std::chrono::system_clock::time_point date_value;

        TemplateValue() : type(STRING), boolean_value(false), number_value(0.0) {}
        TemplateValue(const std::string &str) : type(STRING), string_value(str), boolean_value(false), number_value(0.0) {}
        TemplateValue(const char *str) : type(STRING), string_value(str), boolean_value(false), number_value(0.0) {}
        TemplateValue(bool b) : type(BOOLEAN), boolean_value(b), number_value(0.0) {}
        TemplateValue(int num) : type(NUMBER), boolean_value(false), number_value(static_cast<double>(num)) {}
        TemplateValue(double num) : type(NUMBER), boolean_value(false), number_value(num) {}
        TemplateValue(const std::chrono::system_clock::time_point &date)
            : type(DATE), boolean_value(false), number_value(0.0), date_value(date) {}
        TemplateValue(const std::vector<std::string> &arr) : type(ARRAY), boolean_value(false), number_value(0.0) {
            for(const auto &item : arr) {
                array_value.emplace_back(item);
            }
        }
        TemplateValue(const std::vector<TemplateValue> &arr)
            : type(ARRAY), array_value(arr), boolean_value(false), number_value(0.0) {}
        TemplateValue(const std::map<std::string, TemplateValue> &obj)
            : type(OBJECT), object_value(obj), boolean_value(false), number_value(0.0) {}
        TemplateValue(const std::map<std::string, std::string> &obj)
            : type(OBJECT), boolean_value(false), number_value(0.0) {
            for(const auto &[key, value] : obj) {
                object_value[key] = TemplateValue(value);
            }
        }

        bool is_truthy() const {
            switch(type) {
            case BOOLEAN:
                return boolean_value;
            case STRING:
                return !string_value.empty();
            case ARRAY:
                return !array_value.empty();
            case OBJECT:
                return !object_value.empty();
            case NUMBER:
                return number_value != 0.0;
            case DATE:
                return true;
            }
            return false;
        }

        std::string to_string() const {
            switch(type) {
            case STRING:
                return string_value;
            case BOOLEAN:
                return boolean_value ? "true" : "false";
            case NUMBER:
                if(number_value == static_cast<int>(number_value)) {
                    return std::to_string(static_cast<int>(number_value));
                }
                return std::to_string(number_value);
            case DATE: {
                auto time_t = std::chrono::system_clock::to_time_t(date_value);
                std::stringstream ss;
                ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
                return ss.str();
            }
            case ARRAY:
                return "[array]";
            case OBJECT:
                return "[object]";
            }
            return "";
        }

        TemplateValue get_nested_property(const std::vector<std::string> &path) const;
    };

    class TemplateEngine {
    public:
        static std::string render(const std::string &template_str, const std::map<std::string, TemplateValue> &context);
        static std::string render_with_layout(const std::string &layout_path, const std::string &content_template,
                                              const std::map<std::string, TemplateValue> &context);

        static void register_helper(const std::string &name, TemplateHelper helper);
        static void register_default_helpers();
        static void set_partial_loader(std::function<std::string(const std::string &)> loader);

    private:
        static std::map<std::string, TemplateHelper> helpers_;
        static std::function<std::string(const std::string &)> partial_loader_;

        struct Parser {
            const std::string &template_str;
            std::map<std::string, TemplateValue> context;
            size_t pos;
            std::vector<TemplateError> errors;

            Parser(const std::string &tmpl, const std::map<std::string, TemplateValue> &ctx)
                : template_str(tmpl), context(ctx), pos(0) {}

            std::string parse();
            std::string parse_block();
            std::string parse_if_block();
            std::string parse_each_block();
            std::string parse_for_block();
            std::string parse_variable();
            std::string parse_helper_call();
            std::string parse_partial();

            bool match(const std::string &pattern);
            bool match_keyword(const std::string &keyword);
            std::string extract_until(const std::string &end_pattern);
            std::string extract_condition();
            std::string extract_variable_name();
            std::string extract_collection_name();
            std::string extract_helper_args();
            std::vector<std::string> split_path(const std::string &path) const;
            void skip_whitespace();
            char peek(size_t offset = 0) const;
            char advance();
            bool at_end() const;

            TemplateValue resolve_variable(const std::string &var_name) const;
            TemplateValue resolve_nested_variable(const std::string &path) const;
            std::vector<TemplateValue> parse_helper_arguments(const std::string &args_str) const;

            void add_error(TemplateError::Type type, const std::string &message);
        };
    };

} // namespace ssg::template_engine