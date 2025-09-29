#include "file_utils.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>

namespace ssg::utils {

    std::string FileUtils::read_file(const std::filesystem::path &path) {
        std::ifstream file(path);
        if(!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + path.string());
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    void FileUtils::write_file(const std::filesystem::path &path, const std::string &content) {
        ensure_directory(path.parent_path());

        std::ofstream file(path);
        if(!file.is_open()) {
            throw std::runtime_error("Cannot write file: " + path.string());
        }

        file << content;
    }

    std::vector<std::filesystem::path> FileUtils::get_files_with_extension(const std::filesystem::path &dir,
                                                                           const std::string &ext) {
        std::vector<std::filesystem::path> files;

        if(!std::filesystem::exists(dir)) {
            return files;
        }

        for(const auto &entry : std::filesystem::recursive_directory_iterator(dir)) {
            if(entry.is_regular_file() && entry.path().extension() == ext) {
                files.push_back(entry.path());
            }
        }

        return files;
    }

    std::string FileUtils::path_to_route(const std::filesystem::path &file_path, const std::filesystem::path &base_dir) {
        auto relative = std::filesystem::relative(file_path, base_dir);
        std::string route = "/" + relative.string();

        if(ends_with(route, ".md")) {
            route = route.substr(0, route.length() - 3);
        }

        if(ends_with(route, "/index")) {
            route = route.substr(0, route.length() - 6);
            if(route.empty())
                route = "/";
        }

        std::replace(route.begin(), route.end(), '\\', '/');

        return route;
    }

    std::string FileUtils::path_to_slug(const std::filesystem::path &file_path) { return file_path.stem().string(); }

    void FileUtils::ensure_directory(const std::filesystem::path &dir) {
        if(!dir.empty() && !std::filesystem::exists(dir)) {
            std::filesystem::create_directories(dir);
        }
    }

    std::string StringUtils::trim(const std::string &str) {
        auto start = str.find_first_not_of(" \t\r\n");
        if(start == std::string::npos)
            return "";

        auto end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }

    std::vector<std::string> StringUtils::split(const std::string &str, char delimiter) {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;

        while(std::getline(ss, token, delimiter)) {
            tokens.push_back(trim(token));
        }

        return tokens;
    }

    std::string StringUtils::join(const std::vector<std::string> &parts, const std::string &separator) {
        if(parts.empty())
            return "";

        std::ostringstream oss;
        for(size_t i = 0; i < parts.size(); ++i) {
            if(i > 0)
                oss << separator;
            oss << parts[i];
        }

        return oss.str();
    }

    std::string StringUtils::to_lower(const std::string &str) {
        std::string lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return lower;
    }

    std::string StringUtils::slugify(const std::string &text) {
        std::string slug = to_lower(text);

        std::regex special_chars("[^a-z0-9]+");
        slug = std::regex_replace(slug, special_chars, "-");

        slug = std::regex_replace(slug, std::regex("^-+|-+$"), "");

        return slug;
    }

    std::vector<std::string> StringUtils::parse_array(const std::string &array_str) {
        std::vector<std::string> result;

        std::regex item_regex("\"([^\"]+)\"");

        std::sregex_iterator start(array_str.begin(), array_str.end(), item_regex);
        std::sregex_iterator end;

        for(std::sregex_iterator i = start; i != end; ++i) {
            result.push_back((*i)[1].str());
        }

        return result;
    }

    FrontmatterParser::ParseResult FrontmatterParser::parse(const std::string &input) {
        ParseResult result;
        result.content_start_pos = 0;
        result.content = input;

        if(!starts_with(input, "---")) {
            return result;
        }

        auto end_pos = input.find("\n---\n", 4);
        if(end_pos == std::string::npos) {
            end_pos = input.find("\n---", 4);
            if(end_pos == std::string::npos) {
                return result;
            }
        }

        std::string frontmatter = input.substr(4, end_pos - 4);

        auto lines = StringUtils::split(frontmatter, '\n');
        for(const auto &line : lines) {
            if(!line.empty() && line.find(':') != std::string::npos) {
                parse_line(line, result.metadata);
            }
        }

        result.content_start_pos = end_pos + 4;
        if(result.content_start_pos < input.length()) {
            result.content = StringUtils::trim(input.substr(result.content_start_pos));
        } else {
            result.content = "";
        }

        return result;
    }

    void FrontmatterParser::parse_line(const std::string &line, std::map<std::string, std::string> &metadata) {
        auto colon_pos = line.find(':');
        if(colon_pos == std::string::npos)
            return;

        std::string key = StringUtils::trim(line.substr(0, colon_pos));
        std::string value = StringUtils::trim(line.substr(colon_pos + 1));

        if(starts_with(value, "\"") && ends_with(value, "\"")) {
            value = value.substr(1, value.length() - 2);
        }

        metadata[key] = value;
    }

} // namespace ssg::utils