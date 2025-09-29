#pragma once
#include <cctype>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace toml {

    class Value {
    public:
        using Object = std::unordered_map<std::string, Value>;
        using Array = std::vector<Value>;
        using Number = double;
        using Variant = std::variant<std::nullptr_t, bool, Number, std::string, Array, Object>;

    private:
        Variant _value;

        friend class Parser;

    public:
        Value() : _value(nullptr) {}
        Value(std::nullptr_t) : _value(nullptr) {}
        Value(bool b) : _value(b) {}
        Value(Number n) : _value(n) {}
        Value(const std::string &s) : _value(s) {}
        Value(std::string &&s) : _value(std::move(s)) {}
        Value(const Array &a) : _value(a) {}
        Value(const Object &o) : _value(o) {}

        Value &operator[](const std::string &key) {
            if(!std::holds_alternative<Object>(_value)) {
                _value = Object{};
            }
            return std::get<Object>(_value)[key];
        }

        Value &operator[](size_t index) {
            if(!std::holds_alternative<Array>(_value)) {
                throw std::runtime_error("Not an array");
            }
            return std::get<Array>(_value)[index];
        }

        bool is_null() const { return std::holds_alternative<std::nullptr_t>(_value); }
        bool is_bool() const { return std::holds_alternative<bool>(_value); }
        bool is_number() const { return std::holds_alternative<Number>(_value); }
        bool is_string() const { return std::holds_alternative<std::string>(_value); }
        bool is_array() const { return std::holds_alternative<Array>(_value); }
        bool is_object() const { return std::holds_alternative<Object>(_value); }

        bool get_bool() const { return std::get<bool>(_value); }
        Number get_number() const { return std::get<Number>(_value); }
        const std::string &get_string() const { return std::get<std::string>(_value); }
        const Array &get_array() const { return std::get<Array>(_value); }
        const Object &get_object() const { return std::get<Object>(_value); }

        void serialize(std::string &out) const {
            if(is_null()) {
                out += "null";
            } else if(is_bool()) {
                out += get_bool() ? "true" : "false";
            } else if(is_number()) {
                out += std::to_string(get_number());
            } else if(is_string()) {
                out += '"';
                for(char c : get_string()) {
                    switch(c) {
                    case '"':
                        out += "\\\"";
                        break;
                    case '\\':
                        out += "\\\\";
                        break;
                    case '\b':
                        out += "\\b";
                        break;
                    case '\f':
                        out += "\\f";
                        break;
                    case '\n':
                        out += "\\n";
                        break;
                    case '\r':
                        out += "\\r";
                        break;
                    case '\t':
                        out += "\\t";
                        break;
                    default:
                        if(static_cast<unsigned char>(c) < 0x20 || static_cast<unsigned char>(c) > 0x7E) {
                            char buf[7];
                            snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                            out += buf;
                        } else {
                            out += c;
                        }
                    }
                }
                out += '"';
            } else if(is_array()) {
                out += '[';
                const auto &arr = get_array();
                for(size_t i = 0; i < arr.size(); ++i) {
                    if(i > 0) {
                        out += ", ";
                    }
                    arr[i].serialize(out);
                }
                out += ']';
            } else if(is_object()) {
                out += '{';
                const auto &obj = get_object();
                bool first = true;
                for(const auto &pair : obj) {
                    if(!first) {
                        out += ", ";
                    }
                    first = false;
                    out += pair.first + " = ";
                    pair.second.serialize(out);
                }
                out += '}';
            }
        }
    };

    class Parser {
    public:
        static Value deserialize(const std::string &input) {
            Parser parser(input);
            return parser.parse();
        }

    private:
        const std::string &_input;
        size_t _pos;
        Parser(const std::string &input) : _input(input), _pos(0) {}

        void skip_whitespace_and_comments() {
            while(_pos < _input.size()) {
                if(std::isspace(_input[_pos])) {
                    _pos++;
                } else if(_input[_pos] == '#') {
                    while(_pos < _input.size() && _input[_pos] != '\n') {
                        _pos++;
                    }
                } else {
                    break;
                }
            }
        }

        char peek() const {
            if(_pos >= _input.size()) {
                throw std::runtime_error("Unexpected end of input");
            }
            return _input[_pos];
        }

        char get() {
            if(_pos >= _input.size()) {
                throw std::runtime_error("Unexpected end of input");
            }
            return _input[_pos++];
        }

        Value parse() {
            Value::Object root;
            Value::Object *current_table = &root;

            while(_pos < _input.size()) {
                skip_whitespace_and_comments();
                if(_pos >= _input.size()) {
                    break;
                }

                if(peek() == '[') {
                    current_table = parse_table_header(root);
                } else {
                    parse_key_value(*current_table);
                }
            }
            return Value(root);
        }

        Value::Object *parse_table_header(Value::Object &root) {
            if(get() != '[') {
                throw std::runtime_error("Expected '[' for table header");
            }
            bool is_array = peek() == '[';
            if(is_array) {
                get();
            }

            std::vector<std::string> keys = parse_dotted_keys();
            if(is_array) {
                if(get() != ']' || get() != ']') {
                    throw std::runtime_error("Expected ']]' for array of tables");
                }
            } else {
                if(get() != ']') {
                    throw std::runtime_error("Expected ']' for table header");
                }
            }

            Value::Object *table = &root;
            for(size_t i = 0; i < keys.size() - 1; ++i) {
                auto &key = keys[i];
                if(!table->count(key) || !(*table)[key].is_object()) {
                    (*table)[key] = Value::Object{};
                }
                table = &std::get<Value::Object>((*table)[key]._value);
            }

            const std::string &last_key = keys.back();
            if(is_array) {
                if(!table->count(last_key) || !(*table)[last_key].is_array()) {
                    (*table)[last_key] = Value::Array{};
                }
                auto &arr = std::get<Value::Array>((*table)[last_key]._value);
                arr.emplace_back(Value::Object{});
                return &std::get<Value::Object>(arr.back()._value);
            } else {
                if(!table->count(last_key) || !(*table)[last_key].is_object()) {
                    (*table)[last_key] = Value::Object{};
                }
                return &std::get<Value::Object>((*table)[last_key]._value);
            }
        }

        std::vector<std::string> parse_dotted_keys() {
            std::vector<std::string> keys;
            while(true) {
                skip_whitespace_and_comments();
                keys.push_back(parse_key());
                skip_whitespace_and_comments();
                if(peek() != '.') {
                    break;
                }
                get();
            }
            return keys;
        }

        std::string parse_key() {
            std::string key;
            if(peek() == '"' || peek() == '\'') {
                key = parse_string().get_string();
            } else {
                while(_pos < _input.size() && (std::isalnum(_input[_pos]) || _input[_pos] == '_' || _input[_pos] == '-')) {
                    key += get();
                }
                if(key.empty()) {
                    throw std::runtime_error("Invalid key");
                }
            }
            return key;
        }

        void parse_key_value(Value::Object &table) {
            skip_whitespace_and_comments();
            std::vector<std::string> keys = parse_dotted_keys();
            skip_whitespace_and_comments();
            if(get() != '=') {
                throw std::runtime_error("Expected '=' after key");
            }
            skip_whitespace_and_comments();

            Value value = parse_value();
            Value::Object *target = &table;
            for(size_t i = 0; i < keys.size() - 1; ++i) {
                auto &key = keys[i];
                if(!target->count(key) || !(*target)[key].is_object()) {
                    (*target)[key] = Value::Object{};
                }
                target = &std::get<Value::Object>((*target)[key]._value);
            }
            (*target)[keys.back()] = value;
        }

        Value parse_value() {
            skip_whitespace_and_comments();
            char ch = peek();
            if(ch == 't' || ch == 'f') {
                return parse_bool();
            } else if(ch == '-' || std::isdigit(ch)) {
                return parse_number();
            } else if(ch == '"' || ch == '\'') {
                return parse_string();
            } else if(ch == '[') {
                return parse_array();
            } else if(ch == '{') {
                return parse_inline_table();
            } else if(_input.substr(_pos, 4) == "null") {
                _pos += 4;
                return Value(nullptr);
            }
            throw std::runtime_error("Invalid TOML value");
        }

        Value parse_bool() {
            if(_input.substr(_pos, 4) == "true") {
                _pos += 4;
                return Value(true);
            } else if(_input.substr(_pos, 5) == "false") {
                _pos += 5;
                return Value(false);
            }
            throw std::runtime_error("Invalid boolean value");
        }

        Value parse_number() {
            size_t start = _pos;
            if(_input[_pos] == '-') {
                _pos++;
            }
            while(_pos < _input.size() && std::isdigit(_input[_pos])) {
                _pos++;
            }
            if(_pos < _input.size() && _input[_pos] == '.') {
                _pos++;
                while(_pos < _input.size() && std::isdigit(_input[_pos])) {
                    _pos++;
                }
            }
            if(_pos < _input.size() && (_input[_pos] == 'e' || _input[_pos] == 'E')) {
                _pos++;
                if(_input[_pos] == '+' || _input[_pos] == '-') {
                    _pos++;
                }
                while(_pos < _input.size() && std::isdigit(_input[_pos])) {
                    _pos++;
                }
            }
            try {
                double number = std::stod(_input.substr(start, _pos - start));
                return Value(number);
            } catch(...) { throw std::runtime_error("Invalid number"); }
        }

        Value parse_string() {
            char quote = get();
            if(quote != '"' && quote != '\'') {
                throw std::runtime_error("Expected '\"' or '\'' at start of string");
            }
            std::string result;
            result.reserve(32);
            while(true) {
                char ch = get();
                if(ch == quote) {
                    break;
                } else if(ch == '\\') {
                    char esc = get();
                    switch(esc) {
                    case '"':
                        result += '"';
                        break;
                    case '\\':
                        result += '\\';
                        break;
                    case '/':
                        result += '/';
                        break;
                    case 'b':
                        result += '\b';
                        break;
                    case 'f':
                        result += '\f';
                        break;
                    case 'n':
                        result += '\n';
                        break;
                    case 'r':
                        result += '\r';
                        break;
                    case 't':
                        result += '\t';
                        break;
                    case 'u': {
                        std::string hex;
                        for(int i = 0; i < 4; i++) {
                            hex += get();
                        }
                        char16_t code = static_cast<char16_t>(std::stoi(hex, nullptr, 16));
                        if(code <= 0x7F) {
                            result += static_cast<char>(code);
                        } else {
                            throw std::runtime_error("Unicode characters > 0x7F not supported");
                        }
                        break;
                    }
                    default:
                        throw std::runtime_error("Invalid escape sequence");
                    }
                } else {
                    result += ch;
                }
            }
            return Value(result);
        }

        Value parse_array() {
            if(get() != '[') {
                throw std::runtime_error("Expected '[' at start of array");
            }
            Value::Array arr;
            skip_whitespace_and_comments();
            if(peek() == ']') {
                get();
                return Value(arr);
            }
            while(true) {
                arr.push_back(parse_value());
                skip_whitespace_and_comments();
                char ch = get();
                if(ch == ']') {
                    break;
                } else if(ch != ',') {
                    throw std::runtime_error("Expected ',' or ']' in array");
                }
                skip_whitespace_and_comments();
            }
            return Value(arr);
        }

        Value parse_inline_table() {
            if(get() != '{') {
                throw std::runtime_error("Expected '{' at start of inline table");
            }
            Value::Object obj;
            skip_whitespace_and_comments();
            if(peek() == '}') {
                get();
                return Value(obj);
            }
            while(true) {
                skip_whitespace_and_comments();
                std::string key = parse_key();
                skip_whitespace_and_comments();
                if(get() != '=') {
                    throw std::runtime_error("Expected '=' after key in inline table");
                }
                skip_whitespace_and_comments();
                obj[key] = parse_value();
                skip_whitespace_and_comments();
                char ch = get();
                if(ch == '}') {
                    break;
                } else if(ch != ',') {
                    throw std::runtime_error("Expected ',' or '}' in inline table");
                }
                skip_whitespace_and_comments();
            }
            return Value(obj);
        }
    };

} // namespace toml