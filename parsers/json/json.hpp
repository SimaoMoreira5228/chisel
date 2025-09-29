#pragma once
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace json {

    class Value {
    public:
        using Object = std::unordered_map<std::string, Value>;
        using Array = std::vector<Value>;
        using Number = double;
        using Variant = std::variant<std::nullptr_t, bool, Number, std::string, Array, Object>;

    private:
        Variant _value;

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
                std::string num = std::to_string(get_number());
                num.erase(num.find_last_not_of('0') + 1);
                if(num.back() == '.')
                    num.pop_back();
                out += num;
            } else if(is_string()) {
                out += '"';
                for(char ch : get_string()) {
                    switch(ch) {
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
                        if(static_cast<unsigned char>(ch) < 0x20) {
                            char buf[7];
                            snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(ch));
                            out += buf;
                        } else {
                            out += ch;
                        }
                    }
                }
                out += '"';
            } else if(is_array()) {
                out += '[';
                const auto &arr = get_array();
                for(size_t i = 0; i < arr.size(); i++) {
                    if(i > 0) {
                        out += ',';
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
                        out += ',';
                    }
                    first = false;
                    Value(pair.first).serialize(out);
                    out += ':';
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
            return parser.parse_value();
        }

    private:
        const std::string &_input;
        size_t _pos;
        Parser(const std::string &input) : _input(input), _pos(0) {}

        void skip_whitespace() {
            while(_pos < _input.size() && isspace(_input[_pos])) {
                _pos++;
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

        Value parse_value() {
            skip_whitespace();
            char ch = peek();
            if(ch == 'n') {
                return parse_null();
            } else if(ch == 't' || ch == 'f') {
                return parse_bool();
            } else if(ch == '-' || isdigit(ch)) {
                return parse_number();
            } else if(ch == '"') {
                return parse_string();
            } else if(ch == '[') {
                return parse_array();
            } else if(ch == '{') {
                return parse_object();
            } else {
                throw std::runtime_error("Invalid JSON value");
            }
        }

        Value parse_null() {
            if(_input.substr(_pos, 4) == "null") {
                _pos += 4;
                return Value(nullptr);
            }
            throw std::runtime_error("Invalid null value");
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
            while(_pos < _input.size() && isdigit(_input[_pos])) {
                _pos++;
            }
            if(_pos < _input.size() && _input[_pos] == '.') {
                _pos++;
                while(_pos < _input.size() && isdigit(_input[_pos])) {
                    _pos++;
                }
            }
            if(_pos < _input.size() && (_input[_pos] == 'e' || _input[_pos] == 'E')) {
                _pos++;
                if(_input[_pos] == '+' || _input[_pos] == '-') {
                    _pos++;
                }
                while(_pos < _input.size() && isdigit(_input[_pos])) {
                    _pos++;
                }
            }
            double number = std::stod(_input.substr(start, _pos - start));
            return Value(number);
        }

        Value parse_string() {
            if(get() != '"') {
                throw std::runtime_error("Expected '\"' at start of string");
            }
            std::string result;
            result.reserve(32);
            while(true) {
                char ch = get();
                if(ch == '"') {
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
            skip_whitespace();
            if(peek() == ']') {
                get();
                return Value(arr);
            }
            while(true) {
                arr.push_back(parse_value());
                skip_whitespace();
                char ch = get();
                if(ch == ']') {
                    break;
                } else if(ch != ',') {
                    throw std::runtime_error("Expected ',' or ']' in array");
                }
                skip_whitespace();
            }
            return Value(arr);
        }

        Value parse_object() {
            if(get() != '{') {
                throw std::runtime_error("Expected '{' at start of object");
            }
            Value::Object obj;
            skip_whitespace();
            if(peek() == '}') {
                get();
                return Value(obj);
            }
            while(true) {
                skip_whitespace();
                if(peek() != '"') {
                    throw std::runtime_error("Expected string key in object");
                }
                std::string key = parse_string().get_string();
                skip_whitespace();
                if(get() != ':') {
                    throw std::runtime_error("Expected ':' after key in object");
                }
                skip_whitespace();
                obj[key] = parse_value();
                skip_whitespace();
                char ch = get();
                if(ch == '}') {
                    break;
                } else if(ch != ',') {
                    throw std::runtime_error("Expected ',' or '}' in object");
                }
                skip_whitespace();
            }
            return Value(obj);
        }
    };
} // namespace json