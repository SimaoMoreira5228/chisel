#include "template_engine.hpp"

#include <algorithm>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>

namespace ssg::template_engine {
    std::map<std::string, TemplateHelper> TemplateEngine::helpers_;
    std::function<std::string(const std::string &)> TemplateEngine::partial_loader_;

    TemplateValue TemplateValue::get_nested_property(const std::vector<std::string> &path) const {
        if(path.empty()) {
            return *this;
        }

        if(type != OBJECT) {
            return TemplateValue("");
        }

        const std::string &key = path[0];
        auto it = object_value.find(key);
        if(it == object_value.end()) {
            return TemplateValue("");
        }

        if(path.size() == 1) {
            return it->second;
        }

        std::vector<std::string> remaining_path(path.begin() + 1, path.end());
        return it->second.get_nested_property(remaining_path);
    }

    std::string TemplateEngine::render(const std::string &template_str,
                                       const std::map<std::string, TemplateValue> &context) {
        if(helpers_.empty()) {
            register_default_helpers();
        }

        Parser parser(template_str, context);
        std::string result = parser.parse();

        if(!parser.errors.empty()) {
            std::cerr << "Template rendering errors:\n";
            for(const auto &error : parser.errors) {
                std::cerr << "  Error at position " << error.position << ": " << error.message << "\n";
            }
        }

        return result;
    }

    std::string TemplateEngine::render_with_layout(const std::string &layout_path, const std::string &content_template,
                                                   const std::map<std::string, TemplateValue> &context) {
        if(!partial_loader_) {
            return render(content_template, context);
        }

        std::string layout_content = partial_loader_(layout_path);
        if(layout_content.empty()) {
            return render(content_template, context);
        }

        std::string rendered_content = render(content_template, context);

        auto layout_context = context;
        layout_context["content"] = TemplateValue(rendered_content);

        return render(layout_content, layout_context);
    }

    void TemplateEngine::register_helper(const std::string &name, TemplateHelper helper) { helpers_[name] = helper; }

    void TemplateEngine::set_partial_loader(std::function<std::string(const std::string &)> loader) {
        partial_loader_ = loader;
    }

    std::string TemplateEngine::Parser::parse() {
        std::string result;
        pos = 0;

        while(!at_end()) {
            if(match("{{")) {
                result += parse_block();
            } else {
                result += advance();
            }
        }

        return result;
    }

    std::string TemplateEngine::Parser::parse_block() {
        skip_whitespace();

        if(match_keyword("#if")) {
            return parse_if_block();
        } else if(match_keyword("#each")) {
            return parse_each_block();
        } else if(match_keyword("#for")) {
            return parse_for_block();
        } else if(match_keyword(">")) {
            return parse_partial();
        } else if(peek() == '#') {
            return parse_helper_call();
        } else {
            return parse_variable();
        }
    }

    std::string TemplateEngine::Parser::parse_if_block() {
        skip_whitespace();
        std::string condition = extract_condition();

        if(!match("}}")) {
            return "{{#if " + condition;
        }

        std::string true_content = extract_until("{{/if}}");
        std::string false_content;

        size_t else_pos = true_content.find("{{else}}");
        if(else_pos != std::string::npos) {
            false_content = true_content.substr(else_pos + 8);
            true_content = true_content.substr(0, else_pos);
        }

        TemplateValue condition_value = resolve_variable(condition);

        if(condition_value.is_truthy()) {
            Parser sub_parser(true_content, context);
            return sub_parser.parse();
        } else if(!false_content.empty()) {
            Parser sub_parser(false_content, context);
            return sub_parser.parse();
        }

        return "";
    }

    std::string TemplateEngine::Parser::parse_each_block() {
        skip_whitespace();
        std::string collection_name = extract_collection_name();

        if(!match("}}")) {
            return "{{#each " + collection_name;
        }

        std::string loop_content = extract_until("{{/each}}");
        TemplateValue collection = resolve_variable(collection_name);

        if(collection.type != TemplateValue::ARRAY) {
            return "";
        }

        std::string result;
        for(const auto &item : collection.array_value) {
            std::map<std::string, TemplateValue> loop_context = context;
            loop_context["this"] = item;

            Parser sub_parser(loop_content, loop_context);
            result += sub_parser.parse();
        }

        return result;
    }

    std::string TemplateEngine::Parser::parse_for_block() {
        skip_whitespace();
        std::string var_name = extract_variable_name();
        skip_whitespace();

        if(!match("in")) {
            return "{{#for " + var_name;
        }

        skip_whitespace();
        std::string collection_name = extract_collection_name();

        if(!match("}}")) {
            return "{{#for " + var_name + " in " + collection_name;
        }

        std::string loop_content = extract_until("{{/for}}");
        TemplateValue collection = resolve_variable(collection_name);

        if(collection.type != TemplateValue::ARRAY) {
            return "";
        }

        std::string result;
        for(const auto &item : collection.array_value) {
            std::map<std::string, TemplateValue> loop_context = context;
            loop_context[var_name] = item;

            Parser sub_parser(loop_content, loop_context);
            result += sub_parser.parse();
        }

        return result;
    }

    std::string TemplateEngine::Parser::parse_variable() {
        std::string var_name = extract_variable_name();

        if(!match("}}")) {
            return "{{" + var_name;
        }

        TemplateValue value = resolve_variable(var_name);
        return value.to_string();
    }

    bool TemplateEngine::Parser::match(const std::string &pattern) {
        if(pos + pattern.length() > template_str.length()) {
            return false;
        }

        if(template_str.substr(pos, pattern.length()) == pattern) {
            pos += pattern.length();
            return true;
        }

        return false;
    }

    bool TemplateEngine::Parser::match_keyword(const std::string &keyword) {
        size_t old_pos = pos;
        if(match(keyword)) {
            if(at_end() || std::isspace(peek()) || peek() == '}') {
                return true;
            }
        }
        pos = old_pos;
        return false;
    }

    std::string TemplateEngine::Parser::extract_until(const std::string &end_pattern) {
        size_t start_pos = pos;
        size_t end_pos = template_str.find(end_pattern, pos);

        if(end_pos == std::string::npos) {
            pos = template_str.length();
            return template_str.substr(start_pos);
        }

        std::string content = template_str.substr(start_pos, end_pos - start_pos);
        pos = end_pos + end_pattern.length();
        return content;
    }

    std::string TemplateEngine::Parser::extract_condition() {
        size_t start_pos = pos;
        int brace_count = 0;

        while(!at_end()) {
            char c = peek();
            if(c == '{') {
                brace_count++;
            } else if(c == '}') {
                if(brace_count == 0) {
                    break;
                }
                brace_count--;
            }
            advance();
        }

        return template_str.substr(start_pos, pos - start_pos);
    }

    std::string TemplateEngine::Parser::extract_variable_name() {
        skip_whitespace();
        size_t start_pos = pos;

        while(!at_end()) {
            char c = peek();
            if(std::isalnum(c) || c == '_' || c == '.') {
                advance();
            } else {
                break;
            }
        }

        return template_str.substr(start_pos, pos - start_pos);
    }

    std::string TemplateEngine::Parser::extract_collection_name() { return extract_variable_name(); }

    void TemplateEngine::Parser::skip_whitespace() {
        while(!at_end() && std::isspace(peek())) {
            advance();
        }
    }

    char TemplateEngine::Parser::peek(size_t offset) const {
        if(pos + offset >= template_str.length()) {
            return '\0';
        }
        return template_str[pos + offset];
    }

    char TemplateEngine::Parser::advance() {
        if(at_end()) {
            return '\0';
        }
        return template_str[pos++];
    }

    bool TemplateEngine::Parser::at_end() const { return pos >= template_str.length(); }

    TemplateValue TemplateEngine::Parser::resolve_variable(const std::string &var_name) const {
        if(var_name.find('.') != std::string::npos) {
            return resolve_nested_variable(var_name);
        }

        auto it = context.find(var_name);
        if(it != context.end()) {
            return it->second;
        }

        return TemplateValue("");
    }

    TemplateValue TemplateEngine::Parser::resolve_nested_variable(const std::string &path) const {
        std::vector<std::string> path_parts = split_path(path);
        if(path_parts.empty()) {
            return TemplateValue("");
        }

        auto it = context.find(path_parts[0]);
        if(it == context.end()) {
            return TemplateValue("");
        }

        if(path_parts.size() == 1) {
            return it->second;
        }

        std::vector<std::string> nested_path(path_parts.begin() + 1, path_parts.end());
        return it->second.get_nested_property(nested_path);
    }

    std::string TemplateEngine::Parser::parse_helper_call() {
        advance();
        std::string helper_name = extract_variable_name();
        skip_whitespace();

        std::string args_str = extract_helper_args();

        if(!match("}}")) {
            add_error(TemplateError::SYNTAX_ERROR, "Unclosed helper call: " + helper_name);
            return "{{#" + helper_name + " " + args_str;
        }

        auto helper_it = helpers_.find(helper_name);
        if(helper_it == helpers_.end()) {
            add_error(TemplateError::HELPER_ERROR, "Unknown helper: " + helper_name);
            return "";
        }

        std::vector<TemplateValue> args = parse_helper_arguments(args_str);

        try {
            return helper_it->second(args);
        } catch(const std::exception &e) {
            add_error(TemplateError::HELPER_ERROR, "Helper '" + helper_name + "' error: " + e.what());
            return "";
        }
    }

    std::string TemplateEngine::Parser::parse_partial() {
        skip_whitespace();
        std::string partial_name = extract_variable_name();

        if(!match("}}")) {
            add_error(TemplateError::SYNTAX_ERROR, "Unclosed partial: " + partial_name);
            return "{{>" + partial_name;
        }

        if(!partial_loader_) {
            add_error(TemplateError::PARSE_ERROR, "No partial loader configured");
            return "";
        }

        std::string partial_content = partial_loader_(partial_name);
        if(partial_content.empty()) {
            add_error(TemplateError::PARSE_ERROR, "Partial not found: " + partial_name);
            return "";
        }

        Parser sub_parser(partial_content, context);
        return sub_parser.parse();
    }

    std::string TemplateEngine::Parser::extract_helper_args() {
        size_t start_pos = pos;
        int brace_count = 0;

        while(!at_end()) {
            char c = peek();
            if(c == '{') {
                brace_count++;
            } else if(c == '}') {
                if(brace_count == 0) {
                    break;
                }
                brace_count--;
            }
            advance();
        }

        return template_str.substr(start_pos, pos - start_pos);
    }

    std::vector<std::string> TemplateEngine::Parser::split_path(const std::string &path) const {
        std::vector<std::string> parts;
        std::stringstream ss(path);
        std::string part;

        while(std::getline(ss, part, '.')) {
            if(!part.empty()) {
                parts.push_back(part);
            }
        }

        return parts;
    }

    std::vector<TemplateValue> TemplateEngine::Parser::parse_helper_arguments(const std::string &args_str) const {
        std::vector<TemplateValue> args;
        std::stringstream ss(args_str);
        std::string arg;

        while(ss >> arg) {
            if(arg.front() == '"' && arg.back() == '"') {
                args.emplace_back(arg.substr(1, arg.length() - 2));
            } else if(arg.front() == '\'' && arg.back() == '\'') {
                args.emplace_back(arg.substr(1, arg.length() - 2));
            } else {
                args.push_back(resolve_variable(arg));
            }
        }

        return args;
    }

    void TemplateEngine::Parser::add_error(TemplateError::Type type, const std::string &message) {
        errors.emplace_back(type, message, pos);
    }

    void TemplateEngine::register_default_helpers() {
        register_helper("formatDate", [](const std::vector<TemplateValue> &args) -> std::string {
            if(args.empty())
                return "";

            const auto &date_value = args[0];
            if(date_value.type != TemplateValue::DATE && date_value.type != TemplateValue::STRING) {
                return "";
            }

            std::string format = "%Y-%m-%d";
            if(args.size() > 1 && args[1].type == TemplateValue::STRING) {
                format = args[1].string_value;
            }

            std::chrono::system_clock::time_point time_point;
            if(date_value.type == TemplateValue::DATE) {
                time_point = date_value.date_value;
            } else {
                return date_value.string_value;
            }

            auto time_t = std::chrono::system_clock::to_time_t(time_point);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&time_t), format.c_str());
            return ss.str();
        });

        register_helper("upper", [](const std::vector<TemplateValue> &args) -> std::string {
            if(args.empty())
                return "";
            std::string str = args[0].to_string();
            std::transform(str.begin(), str.end(), str.begin(), ::toupper);
            return str;
        });

        register_helper("lower", [](const std::vector<TemplateValue> &args) -> std::string {
            if(args.empty())
                return "";
            std::string str = args[0].to_string();
            std::transform(str.begin(), str.end(), str.begin(), ::tolower);
            return str;
        });

        register_helper("capitalize", [](const std::vector<TemplateValue> &args) -> std::string {
            if(args.empty())
                return "";
            std::string str = args[0].to_string();
            if(!str.empty()) {
                str[0] = std::toupper(str[0]);
            }
            return str;
        });

        register_helper("length", [](const std::vector<TemplateValue> &args) -> std::string {
            if(args.empty())
                return "0";
            const auto &value = args[0];

            switch(value.type) {
            case TemplateValue::STRING:
                return std::to_string(value.string_value.length());
            case TemplateValue::ARRAY:
                return std::to_string(value.array_value.size());
            case TemplateValue::OBJECT:
                return std::to_string(value.object_value.size());
            default:
                return "0";
            }
        });

        register_helper("truncate", [](const std::vector<TemplateValue> &args) -> std::string {
            if(args.size() < 2)
                return args.empty() ? "" : args[0].to_string();

            std::string str = args[0].to_string();
            if(args[1].type != TemplateValue::NUMBER)
                return str;

            size_t max_length = static_cast<size_t>(args[1].number_value);
            if(str.length() <= max_length)
                return str;

            std::string suffix = "...";
            if(args.size() > 2) {
                suffix = args[2].to_string();
            }

            return str.substr(0, max_length - suffix.length()) + suffix;
        });

        register_helper("join", [](const std::vector<TemplateValue> &args) -> std::string {
            if(args.empty() || args[0].type != TemplateValue::ARRAY)
                return "";

            std::string separator = ", ";
            if(args.size() > 1) {
                separator = args[1].to_string();
            }

            std::string result;
            const auto &array = args[0].array_value;
            for(size_t i = 0; i < array.size(); ++i) {
                if(i > 0)
                    result += separator;
                result += array[i].to_string();
            }

            return result;
        });

        register_helper("add", [](const std::vector<TemplateValue> &args) -> std::string {
            if(args.size() < 2)
                return "0";
            double result = 0.0;

            for(const auto &arg : args) {
                if(arg.type == TemplateValue::NUMBER) {
                    result += arg.number_value;
                } else {
                    try {
                        result += std::stod(arg.to_string());
                    } catch(...) {
                        // Ignore non-numeric values
                    }
                }
            }

            return std::to_string(result);
        });

        register_helper("subtract", [](const std::vector<TemplateValue> &args) -> std::string {
            if(args.size() < 2)
                return "0";

            double result = 0.0;
            if(args[0].type == TemplateValue::NUMBER) {
                result = args[0].number_value;
            } else {
                try {
                    result = std::stod(args[0].to_string());
                } catch(...) { return "0"; }
            }

            for(size_t i = 1; i < args.size(); ++i) {
                if(args[i].type == TemplateValue::NUMBER) {
                    result -= args[i].number_value;
                } else {
                    try {
                        result -= std::stod(args[i].to_string());
                    } catch(...) {
                        // Ignore non-numeric values
                    }
                }
            }

            return std::to_string(result);
        });
    }

} // namespace ssg::template_engine