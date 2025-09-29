#include <atomic>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>

#include "config_cli.hpp"
#include "core/config.hpp"
#include "core/generator.hpp"
#include "http/http_server.hpp"

std::atomic<bool> server_should_stop(false);

void signal_handler(int signal) {
    if(signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n🛑 Shutting down server..." << std::endl;
        server_should_stop = true;
    }
}

bool build_site(const std::filesystem::path &project_path, bool clean_first = false) {
    try {
        std::cout << "🔨 Chisel SSG - Building site from: " << project_path << std::endl;

        std::cout << "\n📖 Loading configuration..." << std::endl;
        std::filesystem::path config_path = project_path / "chisel.config";
        ssg::g_config.load(config_path, project_path);

        if(!ssg::cli::env::is_verbose_enabled()) {
            ssg::g_config.print_summary();
        }

        if(clean_first && std::filesystem::exists(ssg::g_config.get_output_path())) {
            std::cout << "\n🧹 Cleaning output directory..." << std::endl;
            std::filesystem::remove_all(ssg::g_config.get_output_path());
        }

        ssg::SiteGenerator generator(project_path);

        std::cout << "\n🎨 Loading styles..." << std::endl;
        generator.load_styles();

        std::cout << "\n📄 Loading layouts..." << std::endl;
        generator.load_layouts();

        std::cout << "\n⚡ Generating site..." << std::endl;
        generator.generate();

        std::cout << "\n✅ Site built successfully!" << std::endl;
        std::cout << "📁 Output available in: " << ssg::g_config.get_output_path() << std::endl;

        return true;

    } catch(const std::exception &e) {
        std::cerr << "\n❌ Error: " << e.what() << std::endl;
        return false;
    }
}

int main(int argc, char *argv[]) {
    auto args = ssg::cli::ArgumentParser::parse(argc, argv);

    if(args.help) {
        ssg::cli::ArgumentParser::show_help();
        return 0;
    }

    if(args.version) {
        ssg::cli::ArgumentParser::show_version();
        return 0;
    }

    std::string validation_error = ssg::cli::ArgumentParser::validate(args);
    if(!validation_error.empty()) {
        std::cerr << "❌ Error: " << validation_error << std::endl;
        return 1;
    }

    if(args.command == "build") {
        return build_site(args.project_path, args.clean) ? 0 : 1;
    } else if(args.command == "dev") {
        if(!build_site(args.project_path, args.clean)) {
            return 1;
        }

        std::filesystem::path dist_path = ssg::g_config.get_output_path();
        if(!std::filesystem::exists(dist_path)) {
            std::cerr << "❌ Error: Output directory not found. Build the site first." << std::endl;
            return 1;
        }

        try {
            std::signal(SIGINT, signal_handler);
            std::signal(SIGTERM, signal_handler);

            int server_port = ssg::cli::env::get_server_port(args);
            std::string server_host = ssg::cli::env::get_server_host(args);

            std::cout << "🌐 Starting development server at http://" << server_host << ":" << server_port << std::endl;
            http::HttpServerAsync server(server_port, dist_path.string());
            server.start();

            while(server.is_running() && !server_should_stop) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            server.stop();
            std::cout << "✅ Server stopped." << std::endl;
            return 0;

        } catch(const std::exception &e) {
            std::cerr << "\n❌ Server error: " << e.what() << std::endl;
            return 1;
        }
    } else if(args.command == "serve") {
        std::filesystem::path config_path = args.project_path / "chisel.config";
        ssg::g_config.load(config_path, args.project_path);

        std::filesystem::path dist_path = ssg::g_config.get_output_path();
        if(!std::filesystem::exists(dist_path)) {
            std::cerr << "❌ Error: Output directory not found. Build the site first with 'chisel build'." << std::endl;
            return 1;
        }

        try {
            std::signal(SIGINT, signal_handler);
            std::signal(SIGTERM, signal_handler);

            int server_port = ssg::cli::env::get_server_port(args);
            std::string server_host = ssg::cli::env::get_server_host(args);

            std::cout << "🌐 Starting server at http://" << server_host << ":" << server_port << std::endl;
            http::HttpServerAsync server(server_port, dist_path.string());
            server.start();

            while(server.is_running() && !server_should_stop) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            server.stop();
            std::cout << "✅ Server stopped." << std::endl;
            return 0;

        } catch(const std::exception &e) {
            std::cerr << "\n❌ Server error: " << e.what() << std::endl;
            return 1;
        }
    } else {
        std::cerr << "❌ Unknown command: " << args.command << std::endl;
        ssg::cli::ArgumentParser::show_help();
        return 1;
    }
}