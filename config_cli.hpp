#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace ssg {
    namespace cli {
        struct Defaults {
            static constexpr int DEFAULT_PORT = 8080;
            static constexpr const char *DEFAULT_COMMAND = "build";
            static constexpr const char *DEFAULT_HOST = "localhost";
        };

        struct Arguments {
            std::string command = Defaults::DEFAULT_COMMAND;
            std::filesystem::path project_path = std::filesystem::current_path();
            std::optional<int> port;
            std::optional<std::string> host;
            bool verbose = false;
            bool quiet = false;
            bool help = false;
            bool version = false;

            bool watch = false;
            bool clean = false;
            std::optional<std::string> config_file;
        };

        class ArgumentParser {
        public:
            static Arguments parse(int argc, char *argv[]);
            static void show_help();
            static void show_version();
            static std::string validate(const Arguments &args);

        private:
            static int parse_flag(const std::string &arg, const char *next_arg, Arguments &args);
            static std::optional<int> parse_int(const std::string &value);
        };

        namespace env {
            int get_server_port(const Arguments &args);
            std::string get_server_host(const Arguments &args);
            bool is_verbose_enabled();
        } // namespace env

    } // namespace cli

} // namespace ssg
