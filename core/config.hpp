#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "../parsers/toml/toml.hpp"

namespace ssg {
    struct ConfigError : public std::runtime_error {
        explicit ConfigError(const std::string &message) : std::runtime_error(message) {}
    };

    struct SiteConfig {
        std::string name = "My Chisel Site";
        std::string base_url = "";
        std::string description = "";
        std::string author = "";
        std::string language = "en";

        void validate() const;
    };

    struct BuildConfig {
        std::string output_dir = "dist";
        std::string content_dir = "content";
        std::string styles_dir = "styles";
        std::string templates_dir = "templates";
        std::vector<std::string> global_styles = {"base.css"};
        std::map<std::string, std::vector<std::string>> layout_styles = {{"default", {}}, {"post", {"post.css"}}};
        bool minify_css = false;
        bool minify_html = false;

        void validate() const;
    };

    struct DevConfig {
        int port = 3000;
        std::string host = "localhost";
        bool auto_reload = true;
        bool live_reload = false;

        void validate() const;
    };

    struct PerformanceConfig {
        bool enable_cache = true;
        int cache_max_age = 3600;
        size_t max_file_size = 10 * 1024 * 1024;
        bool parallel_processing = true;

        void validate() const;
    };

    class Config {
    public:
        SiteConfig site;
        BuildConfig build;
        DevConfig dev;
        PerformanceConfig performance;

        Config() = default;

        void load(const std::filesystem::path &config_path, const std::filesystem::path &project_root);
        void load_from_string(const std::string &toml_content, const std::filesystem::path &project_root);
        static std::optional<std::string> get_env(const std::string &key);
        static int get_env_int(const std::string &key, int default_value);
        static bool get_env_bool(const std::string &key, bool default_value);
        void resolve_paths(const std::filesystem::path &project_root);
        void validate() const;
        std::filesystem::path get_output_path() const { return output_path_; }
        std::filesystem::path get_content_path() const { return content_path_; }
        std::filesystem::path get_styles_path() const { return styles_path_; }
        std::filesystem::path get_templates_path() const { return templates_path_; }
        void print_summary() const;
        static bool validate_schema(const std::string &toml_content, std::string &error_message);

    private:
        std::filesystem::path output_path_;
        std::filesystem::path content_path_;
        std::filesystem::path styles_path_;
        std::filesystem::path templates_path_;

        void apply_env_overrides();
        void load_site_config(const toml::Value::Object &root);
        void load_build_config(const toml::Value::Object &root);
        void load_dev_config(const toml::Value::Object &root);
        void load_performance_config(const toml::Value::Object &root);
    };

    extern Config g_config;

} // namespace ssg
