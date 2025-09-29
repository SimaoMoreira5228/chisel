#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "../parsers/markdown/markdown.hpp"

namespace ssg {
    struct ContentMeta {
        std::string title;
        std::string layout = "default";
        std::string date;
        std::vector<std::string> classes;
        std::vector<std::string> tags;
        std::map<std::string, std::string> custom_fields;
    };

    struct ContentFile {
        std::filesystem::path source_path;
        std::string route;
        std::string slug;
        ContentMeta meta;
        markdown::Node content_ast;
        std::string rendered_html;

        void generate_route(const std::filesystem::path &content_base_dir);

        void parse_content(const std::string &raw_content);

        void render_html();

    private:
        std::string parse_inline_classes(const std::string &content);
    };

    class ContentManager {
    private:
        std::filesystem::path content_dir;
        std::filesystem::path output_dir;
        std::vector<ContentFile> content_files;

    public:
        ContentManager(const std::filesystem::path &content_path, const std::filesystem::path &output_path);

        void scan_content();

        void process_all();

        const ContentFile *get_content(const std::string &route) const;

        const std::vector<ContentFile> &get_all_content() const;

        void generate_indexes();

        void write_output();
    };
} // namespace ssg
