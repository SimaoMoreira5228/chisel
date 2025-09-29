#pragma once

#include <filesystem>
#include <fstream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace ssg::utils {
    inline bool starts_with(const std::string &str, const std::string &prefix) {
        return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
    }

    inline bool ends_with(const std::string &str, const std::string &suffix) {
        return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    }
    class FileUtils {
    public:
        static std::string read_file(const std::filesystem::path &path);

        static void write_file(const std::filesystem::path &path, const std::string &content);

        static std::vector<std::filesystem::path> get_files_with_extension(const std::filesystem::path &dir,
                                                                           const std::string &ext);

        static std::string path_to_route(const std::filesystem::path &file_path, const std::filesystem::path &base_dir);

        static std::string path_to_slug(const std::filesystem::path &file_path);

        static void ensure_directory(const std::filesystem::path &dir);
    };

    class StringUtils {
    public:
        static std::string trim(const std::string &str);

        static std::vector<std::string> split(const std::string &str, char delimiter);

        static std::string join(const std::vector<std::string> &parts, const std::string &separator);

        static std::string to_lower(const std::string &str);

        static std::string slugify(const std::string &text);

        static std::vector<std::string> parse_array(const std::string &array_str);
    };

    class CSSProcessor {
    public:
        static std::string minify(const std::string &css);

        static std::vector<std::string> extract_selectors(const std::string &css);

        static std::string merge_css(const std::vector<std::string> &css_contents);

        static std::string scope_css(const std::string &css, const std::string &scope_class);
    };

    class FrontmatterParser {
    public:
        struct ParseResult {
            std::map<std::string, std::string> metadata;
            std::string content;
            size_t content_start_pos;
        };

        static ParseResult parse(const std::string &input);

    private:
        static void parse_line(const std::string &line, std::map<std::string, std::string> &metadata);
    };
} // namespace ssg::utils