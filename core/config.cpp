#include "config.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "../parsers/toml/toml.hpp"
#include "../utils/file_utils.hpp"

namespace ssg {
    Config g_config;

    void SiteConfig::validate() const {
        if(name.empty()) {
            throw ConfigError("Site name cannot be empty");
        }

        if(!language.empty() && language.length() != 2 && language.length() != 5) {
            throw ConfigError("Language code must be in format 'en' or 'en-US'");
        }
    }

    void BuildConfig::validate() const {
        if(output_dir.empty()) {
            throw ConfigError("Output directory cannot be empty");
        }

        if(content_dir.empty()) {
            throw ConfigError("Content directory cannot be empty");
        }

        if(styles_dir.empty()) {
            throw ConfigError("Styles directory cannot be empty");
        }

        if(templates_dir.empty()) {
            throw ConfigError("Templates directory cannot be empty");
        }

        if(output_dir == content_dir || output_dir == styles_dir || output_dir == templates_dir) {
            throw ConfigError("Output directory cannot be the same as content, styles, or templates directory");
        }
    }

    void DevConfig::validate() const {
        if(port < 1024 || port > 65535) {
            throw ConfigError("Port must be between 1024 and 65535");
        }

        if(host.empty()) {
            throw ConfigError("Host cannot be empty");
        }
    }

    void PerformanceConfig::validate() const {
        if(cache_max_age < 0) {
            throw ConfigError("Cache max age cannot be negative");
        }

        if(max_file_size == 0) {
            throw ConfigError("Max file size must be greater than 0");
        }
    }

    void Config::load(const std::filesystem::path &config_path, const std::filesystem::path &project_root) {

        std::cout << "📋 Loading configuration from: " << config_path << std::endl;

        if(!std::filesystem::exists(config_path)) {
            std::cout << "📋 No config file found, using defaults with environment overrides" << std::endl;
            apply_env_overrides();
            resolve_paths(project_root);
            validate();
            return;
        }

        try {
            std::string config_content = utils::FileUtils::read_file(config_path);

            std::string schema_error;
            if(!validate_schema(config_content, schema_error)) {
                throw ConfigError("Configuration schema validation failed: " + schema_error);
            }

            load_from_string(config_content, project_root);

        } catch(const std::exception &e) { throw ConfigError("Failed to load configuration: " + std::string(e.what())); }
    }

    void Config::load_from_string(const std::string &toml_content, const std::filesystem::path &project_root) {
        try {
            auto toml_root = toml::Parser::deserialize(toml_content);

            if(!toml_root.is_object()) {
                throw ConfigError("Config file must contain a TOML object at root level");
            }

            const auto &root = toml_root.get_object();

            load_site_config(root);
            load_build_config(root);
            load_dev_config(root);
            load_performance_config(root);
            apply_env_overrides();
            resolve_paths(project_root);
            validate();

            std::cout << "📋 Configuration loaded successfully" << std::endl;

        } catch(const std::exception &e) { throw ConfigError("Failed to parse configuration: " + std::string(e.what())); }
    }

    std::optional<std::string> Config::get_env(const std::string &key) {
        const char *value = std::getenv(key.c_str());
        if(value == nullptr) {
            return std::nullopt;
        }
        return std::string(value);
    }

    int Config::get_env_int(const std::string &key, int default_value) {
        auto env_val = get_env(key);
        if(!env_val) {
            return default_value;
        }

        try {
            return std::stoi(*env_val);
        } catch(const std::exception &) {
            std::cerr << "⚠️  Invalid integer value for " << key << ": " << *env_val << ", using default: " << default_value
                      << std::endl;
            return default_value;
        }
    }

    bool Config::get_env_bool(const std::string &key, bool default_value) {
        auto env_val = get_env(key);
        if(!env_val) {
            return default_value;
        }

        std::string lower_val = *env_val;
        std::transform(lower_val.begin(), lower_val.end(), lower_val.begin(), ::tolower);

        if(lower_val == "true" || lower_val == "1" || lower_val == "yes" || lower_val == "on") {
            return true;
        } else if(lower_val == "false" || lower_val == "0" || lower_val == "no" || lower_val == "off") {
            return false;
        } else {
            std::cerr << "⚠️  Invalid boolean value for " << key << ": " << *env_val
                      << ", using default: " << (default_value ? "true" : "false") << std::endl;
            return default_value;
        }
    }

    void Config::resolve_paths(const std::filesystem::path &project_root) {
        output_path_ = std::filesystem::absolute(project_root / build.output_dir);
        content_path_ = std::filesystem::absolute(project_root / build.content_dir);
        styles_path_ = std::filesystem::absolute(project_root / build.styles_dir);
        templates_path_ = std::filesystem::absolute(project_root / build.templates_dir);
    }

    void Config::validate() const {
        try {
            site.validate();
            build.validate();
            dev.validate();
            performance.validate();
        } catch(const ConfigError &e) { throw ConfigError("Configuration validation failed: " + std::string(e.what())); }
    }

    void Config::print_summary() const {
        std::cout << "\n📋 Configuration Summary:" << std::endl;
        std::cout << "   Site: " << site.name << std::endl;
        std::cout << "   Base URL: " << (site.base_url.empty() ? "(none)" : site.base_url) << std::endl;
        std::cout << "   Language: " << site.language << std::endl;
        std::cout << "   Content: " << content_path_ << std::endl;
        std::cout << "   Styles: " << styles_path_ << std::endl;
        std::cout << "   Templates: " << templates_path_ << std::endl;
        std::cout << "   Output: " << output_path_ << std::endl;
        std::cout << "   Dev Server: " << dev.host << ":" << dev.port << std::endl;
        std::cout << "   Cache: " << (performance.enable_cache ? "enabled" : "disabled") << std::endl;
    }

    bool Config::validate_schema(const std::string &toml_content, std::string &error_message) {
        try {
            auto toml_root = toml::Parser::deserialize(toml_content);

            if(!toml_root.is_object()) {
                error_message = "Root must be an object";
                return false;
            }

            const auto &root = toml_root.get_object();

            const std::vector<std::string> valid_sections = {"site", "build", "dev", "performance", "layout_styles"};

            for(const auto &[key, value] : root) {
                bool found = false;
                for(const auto &valid_section : valid_sections) {
                    if(key == valid_section) {
                        found = true;
                        break;
                    }
                }
                if(!found) {
                    error_message = "Unknown configuration section: " + key;
                    return false;
                }
            }

            return true;

        } catch(const std::exception &e) {
            error_message = "TOML parsing error: " + std::string(e.what());
            return false;
        }
    }

    void Config::apply_env_overrides() {
        if(auto env_val = get_env("CHISEL_SITE_NAME")) {
            site.name = *env_val;
        }
        if(auto env_val = get_env("CHISEL_BASE_URL")) {
            site.base_url = *env_val;
        }
        if(auto env_val = get_env("CHISEL_SITE_DESCRIPTION")) {
            site.description = *env_val;
        }
        if(auto env_val = get_env("CHISEL_SITE_AUTHOR")) {
            site.author = *env_val;
        }
        if(auto env_val = get_env("CHISEL_SITE_LANGUAGE")) {
            site.language = *env_val;
        }
        if(auto env_val = get_env("CHISEL_OUTPUT_DIR")) {
            build.output_dir = *env_val;
        }
        if(auto env_val = get_env("CHISEL_CONTENT_DIR")) {
            build.content_dir = *env_val;
        }
        if(auto env_val = get_env("CHISEL_STYLES_DIR")) {
            build.styles_dir = *env_val;
        }
        if(auto env_val = get_env("CHISEL_TEMPLATES_DIR")) {
            build.templates_dir = *env_val;
        }

        build.minify_css = get_env_bool("CHISEL_MINIFY_CSS", build.minify_css);
        build.minify_html = get_env_bool("CHISEL_MINIFY_HTML", build.minify_html);

        dev.port = get_env_int("CHISEL_DEV_PORT", dev.port);
        if(auto env_val = get_env("CHISEL_DEV_HOST")) {
            dev.host = *env_val;
        }
        dev.auto_reload = get_env_bool("CHISEL_AUTO_RELOAD", dev.auto_reload);
        dev.live_reload = get_env_bool("CHISEL_LIVE_RELOAD", dev.live_reload);

        performance.enable_cache = get_env_bool("CHISEL_ENABLE_CACHE", performance.enable_cache);
        performance.cache_max_age = get_env_int("CHISEL_CACHE_MAX_AGE", performance.cache_max_age);
        performance.parallel_processing = get_env_bool("CHISEL_PARALLEL_PROCESSING", performance.parallel_processing);

        if(auto env_val = get_env("CHISEL_MAX_FILE_SIZE")) {
            try {
                performance.max_file_size = std::stoull(*env_val);
            } catch(const std::exception &) { std::cerr << "⚠️  Invalid max file size: " << *env_val << std::endl; }
        }
    }

    void Config::load_site_config(const toml::Value::Object &root) {
        auto it = root.find("site");
        if(it == root.end() || !it->second.is_object()) {
            return;
        }

        const auto &site_obj = it->second.get_object();

        auto get_string = [&](const std::string &key, std::string &target) {
            auto field_it = site_obj.find(key);
            if(field_it != site_obj.end() && field_it->second.is_string()) {
                target = field_it->second.get_string();
            }
        };

        get_string("name", site.name);
        get_string("base_url", site.base_url);
        get_string("description", site.description);
        get_string("author", site.author);
        get_string("language", site.language);
    }

    void Config::load_build_config(const toml::Value::Object &root) {
        auto it = root.find("build");
        if(it == root.end() || !it->second.is_object()) {
            return;
        }

        const auto &build_obj = it->second.get_object();

        auto get_string = [&](const std::string &key, std::string &target) {
            auto field_it = build_obj.find(key);
            if(field_it != build_obj.end() && field_it->second.is_string()) {
                target = field_it->second.get_string();
            }
        };

        auto get_bool = [&](const std::string &key, bool &target) {
            auto field_it = build_obj.find(key);
            if(field_it != build_obj.end() && field_it->second.is_bool()) {
                target = field_it->second.get_bool();
            }
        };

        get_string("output_dir", build.output_dir);
        get_string("content_dir", build.content_dir);
        get_string("styles_dir", build.styles_dir);
        get_string("templates_dir", build.templates_dir);
        get_bool("minify_css", build.minify_css);
        get_bool("minify_html", build.minify_html);

        auto global_styles_it = build_obj.find("global_styles");
        if(global_styles_it != build_obj.end() && global_styles_it->second.is_array()) {
            const auto &styles_array = global_styles_it->second.get_array();
            build.global_styles.clear();
            for(const auto &style : styles_array) {
                if(style.is_string()) {
                    build.global_styles.push_back(style.get_string());
                }
            }
        }

        auto load_layout_styles = [&](const toml::Value::Object &obj) {
            auto layout_styles_it = obj.find("layout_styles");
            if(layout_styles_it != obj.end() && layout_styles_it->second.is_object()) {
                const auto &layout_obj = layout_styles_it->second.get_object();
                build.layout_styles.clear();

                for(const auto &[layout_name, styles_value] : layout_obj) {
                    if(styles_value.is_array()) {
                        std::vector<std::string> styles;
                        for(const auto &style : styles_value.get_array()) {
                            if(style.is_string()) {
                                styles.push_back(style.get_string());
                            }
                        }
                        build.layout_styles[layout_name] = styles;
                    }
                }
            }
        };

        load_layout_styles(build_obj);
        load_layout_styles(root);
    }

    void Config::load_dev_config(const toml::Value::Object &root) {
        auto it = root.find("dev");
        if(it == root.end() || !it->second.is_object()) {
            return;
        }

        const auto &dev_obj = it->second.get_object();

        auto get_string = [&](const std::string &key, std::string &target) {
            auto field_it = dev_obj.find(key);
            if(field_it != dev_obj.end() && field_it->second.is_string()) {
                target = field_it->second.get_string();
            }
        };

        auto get_int = [&](const std::string &key, int &target) {
            auto field_it = dev_obj.find(key);
            if(field_it != dev_obj.end() && field_it->second.is_number()) {
                target = static_cast<int>(field_it->second.get_number());
            }
        };

        auto get_bool = [&](const std::string &key, bool &target) {
            auto field_it = dev_obj.find(key);
            if(field_it != dev_obj.end() && field_it->second.is_bool()) {
                target = field_it->second.get_bool();
            }
        };

        get_int("port", dev.port);
        get_string("host", dev.host);
        get_bool("auto_reload", dev.auto_reload);
        get_bool("live_reload", dev.live_reload);
    }

    void Config::load_performance_config(const toml::Value::Object &root) {
        auto it = root.find("performance");
        if(it == root.end() || !it->second.is_object()) {
            return;
        }

        const auto &perf_obj = it->second.get_object();

        auto get_bool = [&](const std::string &key, bool &target) {
            auto field_it = perf_obj.find(key);
            if(field_it != perf_obj.end() && field_it->second.is_bool()) {
                target = field_it->second.get_bool();
            }
        };

        auto get_int = [&](const std::string &key, int &target) {
            auto field_it = perf_obj.find(key);
            if(field_it != perf_obj.end() && field_it->second.is_number()) {
                target = static_cast<int>(field_it->second.get_number());
            }
        };

        get_bool("enable_cache", performance.enable_cache);
        get_int("cache_max_age", performance.cache_max_age);
        get_bool("parallel_processing", performance.parallel_processing);

        auto max_file_it = perf_obj.find("max_file_size");
        if(max_file_it != perf_obj.end()) {
            if(max_file_it->second.is_number()) {
                performance.max_file_size = static_cast<size_t>(max_file_it->second.get_number());
            } else if(max_file_it->second.is_string()) {
                std::string size_str = max_file_it->second.get_string();
                try {

                    size_t multiplier = 1;
                    if(utils::ends_with(size_str, "KB")) {
                        multiplier = 1024;
                        size_str = size_str.substr(0, size_str.length() - 2);
                    } else if(utils::ends_with(size_str, "MB")) {
                        multiplier = 1024 * 1024;
                        size_str = size_str.substr(0, size_str.length() - 2);
                    } else if(utils::ends_with(size_str, "GB")) {
                        multiplier = 1024 * 1024 * 1024;
                        size_str = size_str.substr(0, size_str.length() - 2);
                    }

                    size_t base_size = std::stoull(size_str);
                    performance.max_file_size = base_size * multiplier;
                } catch(const std::exception &) {
                    std::cerr << "⚠️  Invalid max_file_size format: " << size_str << std::endl;
                }
            }
        }
    }

} // namespace ssg
