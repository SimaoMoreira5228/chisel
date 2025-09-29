#include "generator.hpp"

#include <iostream>
#include <regex>

#include "../parsers/template/template_engine.hpp"
#include "../utils/file_utils.hpp"
#include "config.hpp"

using ssg::utils::ends_with;
using ssg::utils::starts_with;

namespace ssg {

    SiteGenerator::SiteGenerator(const std::filesystem::path &project_path)
        : project_root(project_path), content_manager(g_config.get_content_path(), g_config.get_output_path()) {

        content_dir = g_config.get_content_path();
        styles_dir = g_config.get_styles_path();
        output_dir = g_config.get_output_path();
    }

    void SiteGenerator::load_styles() {
        stylesheets.clear();

        if(!std::filesystem::exists(styles_dir)) {
            std::cout << "📁 No styles directory found" << std::endl;
            return;
        }

        std::filesystem::path output_styles_dir = output_dir / "styles";
        std::filesystem::create_directories(output_styles_dir);

        auto css_files = utils::FileUtils::get_files_with_extension(styles_dir, ".css");

        for(const auto &css_file : css_files) {
            try {
                StyleSheet stylesheet;
                stylesheet.name = css_file.stem().string();

                std::filesystem::path output_css_file = output_styles_dir / css_file.filename();

                if(css_file != output_css_file) {
                    std::filesystem::copy_file(css_file, output_css_file, std::filesystem::copy_options::overwrite_existing);
                    std::cout << "🎨 Copied stylesheet: " << stylesheet.name << ".css" << std::endl;
                } else {
                    std::cout << "🎨 Stylesheet already in place: " << stylesheet.name << ".css" << std::endl;
                }

                stylesheets[stylesheet.name] = std::move(stylesheet);

            } catch(const std::exception &e) {
                std::cerr << "⚠️  Error copying stylesheet " << css_file << ": " << e.what() << std::endl;
            }
        }
    }

    void SiteGenerator::load_layouts() {
        layouts.clear();

        std::filesystem::path templates_dir = g_config.get_templates_path();

        if(!std::filesystem::exists(templates_dir)) {
            std::cout << "📁 No templates directory found" << std::endl;
            return;
        }

        auto template_files = utils::FileUtils::get_files_with_extension(templates_dir, ".html");

        for(const auto &template_file : template_files) {
            try {
                Layout layout;
                layout.name = template_file.stem().string();
                layout.template_html = utils::FileUtils::read_file(template_file);

                auto layout_styles_it = g_config.build.layout_styles.find(layout.name);
                if(layout_styles_it != g_config.build.layout_styles.end()) {
                    layout.required_styles = layout_styles_it->second;
                }

                layouts[layout.name] = std::move(layout);
                std::cout << "📄 Loaded template: " << layout.name << ".html" << std::endl;

            } catch(const std::exception &e) {
                std::cerr << "⚠️  Error loading template " << template_file << ": " << e.what() << std::endl;
            }
        }
    }

    void SiteGenerator::generate() {
        std::cout << "🚀 Starting site generation..." << std::endl;

        content_manager.scan_content();
        content_manager.process_all();
        content_manager.generate_indexes();

        const auto &all_content = content_manager.get_all_content();

        utils::FileUtils::ensure_directory(output_dir);

        for(const auto &content : all_content) {
            std::string final_html = generate_page(content, content.meta.layout);

            std::filesystem::path output_path = output_dir;

            if(content.route == "/") {
                output_path /= "index.html";
            } else {
                std::string route = content.route;
                if(starts_with(route, "/")) {
                    route = route.substr(1);
                }

                std::string filename = content.source_path.filename().stem().string();
                if(filename == "index") {
                    output_path /= route;
                    utils::FileUtils::ensure_directory(output_path);
                    output_path /= "index.html";
                } else {
                    output_path /= route;
                    output_path += ".html";
                }
            }

            utils::FileUtils::write_file(output_path, final_html);
            std::cout << "✨ Generated: " << output_path.filename() << std::endl;
        }

        std::cout << "🎉 Site generation complete!" << std::endl;
    }

    std::string SiteGenerator::generate_page(const ContentFile &content, const std::string &layout_name) {
        std::string template_html;
        std::vector<std::string> required_styles;

        if(layouts.find(layout_name) != layouts.end()) {
            const auto &layout = layouts.at(layout_name);
            template_html = layout.template_html;
            required_styles = layout.required_styles;
        } else {
            if(layouts.find("default") != layouts.end()) {
                template_html = layouts.at("default").template_html;
            } else {
                template_html = R"(<!DOCTYPE html>
<html><head><title>{{title}}</title><style>{{styles}}</style></head>
<body>{{content}}</body></html>)";
            }
        }

        std::string combined_styles = collect_styles(required_styles, content.meta.classes);

        return apply_template(template_html, content, combined_styles);
    }

    std::string SiteGenerator::collect_styles(const std::vector<std::string> &required_styles,
                                              const std::vector<std::string> &content_classes) {
        std::vector<std::string> link_tags;

        for(const auto &global_style : g_config.build.global_styles) {
            if(stylesheets.find(global_style) != stylesheets.end()) {
                link_tags.push_back("<link rel=\"stylesheet\" href=\"/styles/" + global_style + ".css\">");
            }
        }

        for(const auto &required_style : required_styles) {
            if(stylesheets.find(required_style) != stylesheets.end()) {
                link_tags.push_back("<link rel=\"stylesheet\" href=\"/styles/" + required_style + ".css\">");
            }
        }

        for(const auto &content_class : content_classes) {
            if(stylesheets.find(content_class) != stylesheets.end()) {
                link_tags.push_back("<link rel=\"stylesheet\" href=\"/styles/" + content_class + ".css\">");
            }
        }

        std::string result;
        for(const auto &link : link_tags) {
            if(!result.empty())
                result += "\n    ";
            result += link;
        }
        return result;
    }

    std::string SiteGenerator::apply_template(const std::string &template_html, const ContentFile &content,
                                              const std::string &styles) {
        std::map<std::string, template_engine::TemplateValue> context;

        context["title"] = template_engine::TemplateValue(content.meta.title);
        context["content"] = template_engine::TemplateValue(content.rendered_html);
        context["styles"] = template_engine::TemplateValue(styles);
        context["site_name"] = template_engine::TemplateValue(g_config.site.name);
        context["base_url"] = template_engine::TemplateValue(g_config.site.base_url);
        context["site_description"] = template_engine::TemplateValue(g_config.site.description);
        context["site_author"] = template_engine::TemplateValue(g_config.site.author);
        context["site_language"] = template_engine::TemplateValue(g_config.site.language);
        context["date"] = template_engine::TemplateValue(content.meta.date);

        std::string content_classes = utils::StringUtils::join(content.meta.classes, " ");
        context["content_classes"] = template_engine::TemplateValue(content_classes);

        context["tags"] = template_engine::TemplateValue(content.meta.tags);
        std::string tags_string = utils::StringUtils::join(content.meta.tags, ", ");
        context["tags_string"] = template_engine::TemplateValue(tags_string);

        for(const auto &[key, value] : content.meta.custom_fields) {
            context[key] = template_engine::TemplateValue(value);
        }

        return template_engine::TemplateEngine::render(template_html, context);
    }

    void SiteGenerator::serve(int port) {
        std::cout << "🌐 Development server not implemented yet" << std::endl;
        std::cout << "📁 Serve files from: " << output_dir << std::endl;
    }

} // namespace ssg