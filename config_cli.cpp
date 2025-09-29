#include "config_cli.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <sstream>

namespace ssg {
    namespace cli {

        Arguments ArgumentParser::parse(int argc, char *argv[]) {
            Arguments args;

            for(int i = 1; i < argc; ++i) {
                std::string arg = argv[i];
                const char *next_arg = (i + 1 < argc) ? argv[i + 1] : nullptr;

                if(arg[0] == '-') {
                    int consumed = parse_flag(arg, next_arg, args);
                    i += consumed - 1;
                } else if(args.command == Defaults::DEFAULT_COMMAND && i == 1) {
                    if(arg == "build" || arg == "dev" || arg == "serve" || arg == "help" || arg == "version") {
                        args.command = arg;
                    } else {
                        args.project_path = std::filesystem::absolute(arg);
                    }
                } else if(i == 2 && (args.command == "build" || args.command == "dev" || args.command == "serve")) {
                    args.project_path = std::filesystem::absolute(arg);
                } else {
                    std::cerr << "⚠️  Warning: Ignoring unknown argument: " << arg << std::endl;
                }
            }

            if(args.command == "help") {
                args.help = true;
            } else if(args.command == "version") {
                args.version = true;
            }

            return args;
        }

        int ArgumentParser::parse_flag(const std::string &arg, const char *next_arg, Arguments &args) {
            if(arg == "--help" || arg == "-h") {
                args.help = true;
                return 1;
            }

            if(arg == "--version" || arg == "-v") {
                args.version = true;
                return 1;
            }

            if(arg == "--verbose") {
                args.verbose = true;
                return 1;
            }

            if(arg == "--quiet" || arg == "-q") {
                args.quiet = true;
                return 1;
            }

            if(arg == "--watch" || arg == "-w") {
                args.watch = true;
                return 1;
            }

            if(arg == "--clean" || arg == "-c") {
                args.clean = true;
                return 1;
            }

            if(arg == "--port" || arg == "-p") {
                if(next_arg == nullptr) {
                    throw std::runtime_error("--port requires a value");
                }
                auto port = parse_int(next_arg);
                if(!port) {
                    throw std::runtime_error("Invalid port number: " + std::string(next_arg));
                }
                args.port = *port;
                return 2;
            }

            if(arg == "--host") {
                if(next_arg == nullptr) {
                    throw std::runtime_error("--host requires a value");
                }
                args.host = next_arg;
                return 2;
            }

            if(arg == "--config") {
                if(next_arg == nullptr) {
                    throw std::runtime_error("--config requires a value");
                }
                args.config_file = next_arg;
                return 2;
            }

            throw std::runtime_error("Unknown flag: " + arg);
        }

        std::optional<int> ArgumentParser::parse_int(const std::string &value) {
            try {
                int result = std::stoi(value);
                return result;
            } catch(const std::exception &) { return std::nullopt; }
        }

        void ArgumentParser::show_help() {
            std::cout << "🔨 Chisel Static Site Generator\n" << std::endl;

            std::cout << "Usage:" << std::endl;
            std::cout << "  chisel [project_path]              Build the site (default command)" << std::endl;
            std::cout << "  chisel build [project_path]        Build the site" << std::endl;
            std::cout << "  chisel dev [project_path]          Build and serve in development mode" << std::endl;
            std::cout << "  chisel serve [project_path]        Serve the built site" << std::endl;
            std::cout << "  chisel help                        Show this help message" << std::endl;
            std::cout << "  chisel version                     Show version information" << std::endl;

            std::cout << "\nOptions:" << std::endl;
            std::cout << "  -h, --help                         Show this help message" << std::endl;
            std::cout << "  -v, --version                      Show version information" << std::endl;
            std::cout << "  -p, --port <port>                  Port for dev/serve commands (default: from config or "
                      << Defaults::DEFAULT_PORT << ")" << std::endl;
            std::cout << "  --host <host>                      Host for dev/serve commands (default: from config or "
                      << Defaults::DEFAULT_HOST << ")" << std::endl;
            std::cout << "  -c, --clean                        Clean output directory before build" << std::endl;
            std::cout << "  -w, --watch                        Watch for file changes (dev mode only)" << std::endl;
            std::cout << "  --config <path>                    Path to configuration file (default: chisel.config)"
                      << std::endl;
            std::cout << "  --verbose                          Enable verbose logging" << std::endl;
            std::cout << "  -q, --quiet                        Suppress non-error output" << std::endl;

            std::cout << "\nEnvironment Variables:" << std::endl;
            std::cout << "  CHISEL_DEV_PORT                    Override development server port" << std::endl;
            std::cout << "  CHISEL_DEV_HOST                    Override development server host" << std::endl;
            std::cout << "  CHISEL_OUTPUT_DIR                  Override output directory" << std::endl;
            std::cout << "  CHISEL_CONTENT_DIR                 Override content directory" << std::endl;
            std::cout << "  CHISEL_STYLES_DIR                  Override styles directory" << std::endl;
            std::cout << "  CHISEL_TEMPLATES_DIR               Override templates directory" << std::endl;
            std::cout << "  CHISEL_SITE_NAME                   Override site name" << std::endl;
            std::cout << "  CHISEL_BASE_URL                    Override base URL" << std::endl;
            std::cout << "  CHISEL_VERBOSE                     Enable verbose logging (true/false)" << std::endl;
            std::cout << "  CI                                 Detected CI environment flag" << std::endl;

            std::cout << "\nExamples:" << std::endl;
            std::cout << "  chisel                             Build current directory" << std::endl;
            std::cout << "  chisel /path/to/project            Build specific project" << std::endl;
            std::cout << "  chisel dev --port 4000             Start dev server on port 4000" << std::endl;
            std::cout << "  chisel build --clean               Clean and build" << std::endl;
            std::cout << "  chisel serve --host 0.0.0.0        Serve on all interfaces" << std::endl;
        }

        void ArgumentParser::show_version() {
            std::cout << "🔨 Chisel Static Site Generator" << std::endl;
            std::cout << "Version: 0.1.0" << std::endl;
            std::cout << "Built with C++17" << std::endl;
            std::cout << "\nFeatures:" << std::endl;
            std::cout << "  ⚡ Fast static site generation" << std::endl;
            std::cout << "  📝 Markdown content processing" << std::endl;
            std::cout << "  🎨 CSS styling support" << std::endl;
            std::cout << "  🌐 Built-in development server" << std::endl;
            std::cout << "  ⚙️  Flexible configuration system" << std::endl;
        }

        std::string ArgumentParser::validate(const Arguments &args) {
            if(args.port && (*args.port < 1024 || *args.port > 65535)) {
                return "Port must be between 1024 and 65535";
            }

            if(args.verbose && args.quiet) {
                return "Cannot use both --verbose and --quiet flags";
            }

            if(!std::filesystem::exists(args.project_path)) {
                return "Project path does not exist: " + args.project_path.string();
            }

            if(!std::filesystem::is_directory(args.project_path)) {
                return "Project path is not a directory: " + args.project_path.string();
            }

            if(args.config_file && !std::filesystem::exists(*args.config_file)) {
                return "Config file does not exist: " + *args.config_file;
            }

            return "";
        }

        namespace env {

            int get_server_port(const Arguments &args) {
                if(args.port) {
                    return *args.port;
                }

                const char *env_port = std::getenv("CHISEL_DEV_PORT");
                if(env_port) {
                    try {
                        int port = std::stoi(env_port);
                        if(port >= 1024 && port <= 65535) {
                            return port;
                        }
                    } catch(const std::exception &) {
                        std::cerr << "⚠️  Invalid CHISEL_DEV_PORT value: " << env_port << std::endl;
                    }
                }

                return Defaults::DEFAULT_PORT;
            }

            std::string get_server_host(const Arguments &args) {
                if(args.host) {
                    return *args.host;
                }

                const char *env_host = std::getenv("CHISEL_DEV_HOST");
                if(env_host) {
                    return std::string(env_host);
                }

                return Defaults::DEFAULT_HOST;
            }

            bool is_verbose_enabled() {
                const char *env_verbose = std::getenv("CHISEL_VERBOSE");
                if(env_verbose) {
                    std::string value = env_verbose;
                    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
                    return value == "true" || value == "1" || value == "yes" || value == "on";
                }
                return false;
            }

        } // namespace env

    } // namespace cli
} // namespace ssg