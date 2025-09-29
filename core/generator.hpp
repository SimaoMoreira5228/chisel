#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "../parsers/template/template_engine.hpp"
#include "content.hpp"

namespace ssg {
    struct StyleSheet {
        std::string name;
        std::string content;
    };

    struct Layout {
        std::string name;
        std::string template_html;
        std::vector<std::string> required_styles;
    };

    class SiteGenerator {
    private:
        std::filesystem::path project_root;
        std::filesystem::path content_dir;
        std::filesystem::path styles_dir;
        std::filesystem::path output_dir;

        ContentManager content_manager;
        std::map<std::string, StyleSheet> stylesheets;
        std::map<std::string, Layout> layouts;

    public:
        SiteGenerator(const std::filesystem::path &project_path);

        void load_styles();

        void load_layouts();

        void generate();

        std::string generate_page(const ContentFile &content, const std::string &layout_name = "default");

        std::string collect_styles(const std::vector<std::string> &required_styles,
                                   const std::vector<std::string> &content_classes);

        void serve(int port = 3000);

    private:
        ContentMeta parse_frontmatter(const std::string &content, size_t &content_start);

        std::string apply_template(const std::string &template_html, const ContentFile &content, const std::string &styles);
    };
} // namespace ssg
