#include "content.hpp"

#include <iostream>
#include <regex>

#include "../parsers/markdown/markdown.hpp"
#include "../utils/file_utils.hpp"

using ssg::utils::ends_with;
using ssg::utils::starts_with;

namespace ssg {

    void ContentFile::generate_route(const std::filesystem::path &content_base_dir) {
        route = utils::FileUtils::path_to_route(source_path, content_base_dir);
        slug = utils::FileUtils::path_to_slug(source_path);
    }

    void ContentFile::parse_content(const std::string &raw_content) {
        auto frontmatter_result = utils::FrontmatterParser::parse(raw_content);

        for(const auto &[key, value] : frontmatter_result.metadata) {
            if(key == "title") {
                meta.title = value;
            } else if(key == "layout") {
                meta.layout = value;
            } else if(key == "date") {
                meta.date = value;
            } else if(key == "classes") {
                if(starts_with(value, "[") && ends_with(value, "]")) {
                    meta.classes = utils::StringUtils::parse_array(value);
                } else {
                    meta.classes = {value};
                }
            } else if(key == "tags") {
                if(starts_with(value, "[") && ends_with(value, "]")) {
                    meta.tags = utils::StringUtils::parse_array(value);
                } else {
                    meta.tags = {value};
                }
            } else {
                meta.custom_fields[key] = value;
            }
        }

        std::string content = frontmatter_result.content;
        content = parse_inline_classes(content);

        content_ast = markdown::Deserializer::deserialize(content);
    }

    void ContentFile::render_html() { rendered_html = markdown::Serializer::html(content_ast); }

    std::string ContentFile::parse_inline_classes(const std::string &content) {
        std::string processed = content;

        std::regex inline_class_regex(R"((#+\s*[^-]+)\s*---\s*classes\[([^\]]+)\])");
        std::smatch match;

        while(std::regex_search(processed, match, inline_class_regex)) {
            std::string heading = match[1].str();
            std::string classes_str = match[2].str();

            std::vector<std::string> classes;
            std::regex class_regex("\"([^\"]+)\"");
            std::sregex_iterator start(classes_str.begin(), classes_str.end(), class_regex);
            std::sregex_iterator end;

            for(std::sregex_iterator i = start; i != end; ++i) {
                classes.push_back((*i)[1].str());
                meta.classes.push_back((*i)[1].str());
            }

            processed = std::regex_replace(processed, inline_class_regex, heading, std::regex_constants::format_first_only);
        }

        return processed;
    }

    ContentManager::ContentManager(const std::filesystem::path &content_path, const std::filesystem::path &output_path)
        : content_dir(content_path), output_dir(output_path) {}

    void ContentManager::scan_content() {
        content_files.clear();

        auto md_files = utils::FileUtils::get_files_with_extension(content_dir, ".md");

        for(const auto &file_path : md_files) {
            try {
                ContentFile content_file;
                content_file.source_path = file_path;
                content_file.generate_route(content_dir);

                std::string raw_content = utils::FileUtils::read_file(file_path);
                content_file.parse_content(raw_content);
                content_file.render_html();

                content_files.push_back(std::move(content_file));

                std::cout << "📄 Loaded: " << file_path.filename() << " -> " << content_file.route << std::endl;

            } catch(const std::exception &e) {
                std::cerr << "⚠️  Error processing " << file_path << ": " << e.what() << std::endl;
            }
        }
    }

    void ContentManager::process_all() {
        for(auto &content : content_files) {
            content.render_html();
        }
    }

    const ContentFile *ContentManager::get_content(const std::string &route) const {
        for(const auto &content : content_files) {
            if(content.route == route) {
                return &content;
            }
        }
        return nullptr;
    }

    const std::vector<ContentFile> &ContentManager::get_all_content() const { return content_files; }

    void ContentManager::generate_indexes() {
        std::map<std::string, std::vector<const ContentFile *>> directories;

        for(const auto &content : content_files) {
            std::filesystem::path route_path(content.route);
            if(route_path.has_parent_path() && route_path.parent_path() != "/") {
                std::string dir = route_path.parent_path().string();
                directories[dir].push_back(&content);
            }
        }

        for(const auto &[dir, files] : directories) {
            if(files.size() > 1) {
                ContentFile index_file;
                index_file.route = dir;
                index_file.slug = "index";
                index_file.meta.title = "Index of " + dir;
                index_file.meta.layout = "default";

                std::ostringstream index_content;
                index_content << "# " << index_file.meta.title << "\n\n";

                for(const auto *file : files) {
                    index_content << "- [" << file->meta.title << "](" << file->route << ")\n";
                }

                index_file.content_ast = markdown::Deserializer::deserialize(index_content.str());
                index_file.render_html();

                content_files.push_back(std::move(index_file));
            }
        }
    }

    void ContentManager::write_output() {
        utils::FileUtils::ensure_directory(output_dir);

        for(const auto &content : content_files) {
            std::filesystem::path output_path = output_dir;

            if(content.route == "/") {
                output_path /= "index.html";
            } else {
                std::string route = content.route;
                if(starts_with(route, "/")) {
                    route = route.substr(1);
                }
                output_path /= route;
                output_path += ".html";
            }

            utils::FileUtils::write_file(output_path, content.rendered_html);
            std::cout << "📝 Written: " << output_path << std::endl;
        }
    }

} // namespace ssg