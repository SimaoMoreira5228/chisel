#pragma once

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#define poll WSAPoll
#define CLOSE_SOCKET closesocket
#define SOCKET_TYPE SOCKET
#define INVALID_SOCKET_TYPE INVALID_SOCKET
#define SOCKET_ERROR_CODE SOCKET_ERROR
#define SET_NONBLOCK(fd)                                                                                                    \
    {                                                                                                                       \
        u_long mode = 1;                                                                                                    \
        ioctlsocket(fd, FIONBIO, &mode);                                                                                    \
    }
#define GET_LAST_ERROR WSAGetLastError()
#else
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#define CLOSE_SOCKET close
#define SOCKET_TYPE int
#define INVALID_SOCKET_TYPE -1
#define SOCKET_ERROR_CODE -1
#define SET_NONBLOCK(fd)                                                                                                    \
    {                                                                                                                       \
        int flags = fcntl(fd, F_GETFL, 0);                                                                                  \
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);                                                                             \
    }
#define GET_LAST_ERROR errno
#endif

namespace http {
    enum class HttpStatus {
        OK = 200,
        NOT_MODIFIED = 304,
        BAD_REQUEST = 400,
        NOT_FOUND = 404,
        METHOD_NOT_ALLOWED = 405,
        INTERNAL_SERVER_ERROR = 500
    };

    struct CacheEntry {
        std::string content;
        std::string content_type;
        std::string etag;
        std::filesystem::file_time_type last_modified;
        std::chrono::system_clock::time_point cached_at;
        size_t file_size;
    };

    struct HttpRequest {
        std::string method;
        std::string path;
        std::string version;
        std::unordered_map<std::string, std::string> headers;
        std::string body;

        std::string get_if_none_match() const {
            auto it = headers.find("if-none-match");
            return it != headers.end() ? it->second : "";
        }
    };

    class HttpServerAsync {
    public:
        HttpServerAsync(int port, const std::string &root_dir)
            : port_(port),
              root_dir_(root_dir),
              server_fd_(INVALID_SOCKET_TYPE),
              running_(false),
              current_cache_size_(0),
              cache_max_size_(50 * 1024 * 1024),
              cache_ttl_(std::chrono::minutes(30)) {
            init_mime_types();
        }

        ~HttpServerAsync() { stop(); }

        void start() {
#ifdef _WIN32
            WSADATA wsaData;
            if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
                throw std::runtime_error("WSAStartup failed");
            }
#endif

            server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
            if(server_fd_ == INVALID_SOCKET_TYPE) {
                throw std::runtime_error("Failed to create socket: " + std::to_string(GET_LAST_ERROR));
            }

            int opt = 1;
            if(setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, (const char *) &opt, sizeof(opt)) == SOCKET_ERROR_CODE) {
                throw std::runtime_error("Failed to set socket options: " + std::to_string(GET_LAST_ERROR));
            }

            struct sockaddr_in address;
            memset(&address, 0, sizeof(address));
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = INADDR_ANY;
            address.sin_port = htons(port_);
            if(bind(server_fd_, (struct sockaddr *) &address, sizeof(address)) == SOCKET_ERROR_CODE) {
                throw std::runtime_error("Failed to bind socket: " + std::to_string(GET_LAST_ERROR));
            }

            if(listen(server_fd_, 10) == SOCKET_ERROR_CODE) {
                throw std::runtime_error("Failed to listen: " + std::to_string(GET_LAST_ERROR));
            }

            SET_NONBLOCK(server_fd_);

            running_ = true;
            server_thread_ = std::thread(&HttpServerAsync::run_event_loop, this);

            std::cout << "🌐 Development server running at http://localhost:" << port_ << std::endl;
            std::cout << "📁 Serving files from: " << root_dir_ << std::endl;
            std::cout << "🚀 Features: MIME detection, ETag caching, Path resolution, Error handling" << std::endl;
            std::cout << "💾 Cache: " << (cache_max_size_ / (1024 * 1024)) << "MB max, " << (cache_ttl_.count()) << "min TTL"
                      << std::endl;
            std::cout << "Press Ctrl+C to stop..." << std::endl;
        }

        void stop() {
            running_ = false;
            if(server_fd_ != INVALID_SOCKET_TYPE) {
                CLOSE_SOCKET(server_fd_);
                server_fd_ = INVALID_SOCKET_TYPE;
            }
            if(server_thread_.joinable()) {
                server_thread_.join();
            }
#ifdef _WIN32
            WSACleanup();
#endif
        }

        bool is_running() const { return running_; }

    private:
        int port_;
        std::string root_dir_;
        SOCKET_TYPE server_fd_;
        bool running_;
        std::thread server_thread_;

        std::unordered_map<std::string, CacheEntry> file_cache_;
        size_t current_cache_size_;
        size_t cache_max_size_;
        std::chrono::minutes cache_ttl_;

        std::unordered_map<std::string, std::string> mime_types_;

        struct Client {
            SOCKET_TYPE fd;
            std::string request_buffer;
        };

        void run_event_loop() {
            std::vector<pollfd> fds;
            std::vector<Client> clients;
            fds.push_back({server_fd_, POLLIN, 0});

            while(running_) {
                int ret = poll(fds.data(), static_cast<nfds_t>(fds.size()), 1000);
                if(ret == SOCKET_ERROR_CODE) {
                    if(running_) {
                        std::cerr << "Poll failed: " << GET_LAST_ERROR << std::endl;
                    }
                    continue;
                }
                if(ret == 0)
                    continue;

                for(size_t i = 0; i < fds.size(); ++i) {
                    if(!running_)
                        break;

                    if(fds[i].revents & (POLLIN | POLLERR | POLLHUP)) {
                        if(i == 0) {
                            struct sockaddr_in client_addr;
                            socklen_t addr_len = sizeof(client_addr);
                            SOCKET_TYPE client_fd = accept(server_fd_, (struct sockaddr *) &client_addr, &addr_len);
                            if(client_fd == INVALID_SOCKET_TYPE) {
                                if(running_) {
                                    std::cerr << "Accept failed: " << GET_LAST_ERROR << std::endl;
                                }
                                continue;
                            }
                            SET_NONBLOCK(client_fd);
                            fds.push_back({client_fd, POLLIN, 0});
                            clients.push_back({client_fd, ""});
                        } else {
                            char buffer[1024];
                            memset(buffer, 0, sizeof(buffer));
                            int bytes_read = recv(fds[i].fd, buffer, sizeof(buffer) - 1, 0);
                            if(bytes_read == SOCKET_ERROR_CODE || bytes_read == 0) {
                                CLOSE_SOCKET(fds[i].fd);
                                fds.erase(fds.begin() + i);
                                clients.erase(clients.begin() + (i - 1));
                                --i;
                                continue;
                            }

                            clients[i - 1].request_buffer.append(buffer, bytes_read);

                            if(clients[i - 1].request_buffer.find("\r\n\r\n") != std::string::npos) {
                                try {
                                    HttpRequest request = parse_request(clients[i - 1].request_buffer);
                                    std::string response = handle_request(request);

                                    ssize_t bytes_sent =
                                        send(fds[i].fd, response.c_str(), static_cast<int>(response.size()), 0);
                                    if(bytes_sent == SOCKET_ERROR_CODE) {
                                        std::cerr << "Failed to send response: " << GET_LAST_ERROR << std::endl;
                                    }
                                } catch(const std::exception &e) {
                                    std::cerr << "Error handling request: " << e.what() << std::endl;
                                    std::string error_response = generate_error_response(HttpStatus::INTERNAL_SERVER_ERROR);
                                    send(fds[i].fd, error_response.c_str(), static_cast<int>(error_response.size()), 0);
                                }

                                CLOSE_SOCKET(fds[i].fd);
                                fds.erase(fds.begin() + i);
                                clients.erase(clients.begin() + (i - 1));
                                --i;
                            }
                        }
                    }
                }
            }

            for(auto &client : clients) {
                CLOSE_SOCKET(client.fd);
            }
        }

        HttpRequest parse_request(const std::string &request_str) {
            HttpRequest request;
            std::istringstream stream(request_str);
            std::string line;

            if(std::getline(stream, line)) {
                std::istringstream request_line(line);
                request_line >> request.method >> request.path >> request.version;
            }

            while(std::getline(stream, line) && line != "\r") {
                if(line.back() == '\r')
                    line.pop_back();

                size_t colon_pos = line.find(':');
                if(colon_pos != std::string::npos) {
                    std::string key = line.substr(0, colon_pos);
                    std::string value = line.substr(colon_pos + 1);

                    key.erase(0, key.find_first_not_of(" \t"));
                    key.erase(key.find_last_not_of(" \t") + 1);
                    value.erase(0, value.find_first_not_of(" \t"));
                    value.erase(value.find_last_not_of(" \t") + 1);

                    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
                    request.headers[key] = value;
                }
            }

            std::string body_line;
            while(std::getline(stream, body_line)) {
                request.body += body_line + "\n";
            }

            return request;
        }

        std::string resolve_path(const std::string &path) {
            std::string resolved_path = path;

            if(resolved_path == "/") {
                resolved_path = "/index.html";
            }

            std::string decoded_path;
            for(size_t i = 0; i < resolved_path.length(); ++i) {
                if(resolved_path[i] == '%' && i + 2 < resolved_path.length()) {
                    std::string hex = resolved_path.substr(i + 1, 2);
                    char decoded_char = static_cast<char>(std::stoul(hex, nullptr, 16));
                    decoded_path += decoded_char;
                    i += 2;
                } else {
                    decoded_path += resolved_path[i];
                }
            }
            resolved_path = decoded_path;

            if(resolved_path.find("..") != std::string::npos) {
                return "/index.html";
            }

            if(resolved_path.find('.') == std::string::npos && resolved_path.back() != '/') {
                std::string index_path = resolved_path + "/index.html";
                std::string full_index_path = root_dir_ + index_path;
                if(std::filesystem::exists(full_index_path)) {
                    return index_path;
                }

                std::string html_path = resolved_path + ".html";
                std::string full_html_path = root_dir_ + html_path;
                if(std::filesystem::exists(full_html_path)) {
                    return html_path;
                }
            }

            return resolved_path;
        }

        void init_mime_types() {
            mime_types_[".html"] = "text/html; charset=utf-8";
            mime_types_[".htm"] = "text/html; charset=utf-8";
            mime_types_[".css"] = "text/css; charset=utf-8";
            mime_types_[".js"] = "application/javascript; charset=utf-8";
            mime_types_[".mjs"] = "application/javascript; charset=utf-8";
            mime_types_[".json"] = "application/json; charset=utf-8";
            mime_types_[".xml"] = "application/xml; charset=utf-8";
            mime_types_[".txt"] = "text/plain; charset=utf-8";
            mime_types_[".md"] = "text/markdown; charset=utf-8";
            mime_types_[".csv"] = "text/csv; charset=utf-8";

            mime_types_[".png"] = "image/png";
            mime_types_[".jpg"] = "image/jpeg";
            mime_types_[".jpeg"] = "image/jpeg";
            mime_types_[".gif"] = "image/gif";
            mime_types_[".svg"] = "image/svg+xml";
            mime_types_[".ico"] = "image/x-icon";
            mime_types_[".webp"] = "image/webp";
            mime_types_[".bmp"] = "image/bmp";
            mime_types_[".tiff"] = "image/tiff";

            mime_types_[".mp3"] = "audio/mpeg";
            mime_types_[".wav"] = "audio/wav";
            mime_types_[".ogg"] = "audio/ogg";
            mime_types_[".mp4"] = "video/mp4";
            mime_types_[".webm"] = "video/webm";
            mime_types_[".avi"] = "video/x-msvideo";

            mime_types_[".woff"] = "font/woff";
            mime_types_[".woff2"] = "font/woff2";
            mime_types_[".ttf"] = "font/ttf";
            mime_types_[".otf"] = "font/otf";
            mime_types_[".eot"] = "application/vnd.ms-fontobject";

            mime_types_[".pdf"] = "application/pdf";
            mime_types_[".zip"] = "application/zip";
            mime_types_[".tar"] = "application/x-tar";
            mime_types_[".gz"] = "application/gzip";

            mime_types_[".wasm"] = "application/wasm";
        }

        std::string get_content_type(const std::string &path) {
            std::filesystem::path file_path(path);
            std::string ext = file_path.extension().string();

            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            auto it = mime_types_.find(ext);
            if(it != mime_types_.end()) {
                return it->second;
            }

            return "application/octet-stream";
        }

        std::string get_status_text(HttpStatus status) {
            switch(status) {
            case HttpStatus::OK:
                return "OK";
            case HttpStatus::NOT_MODIFIED:
                return "Not Modified";
            case HttpStatus::BAD_REQUEST:
                return "Bad Request";
            case HttpStatus::NOT_FOUND:
                return "Not Found";
            case HttpStatus::METHOD_NOT_ALLOWED:
                return "Method Not Allowed";
            case HttpStatus::INTERNAL_SERVER_ERROR:
                return "Internal Server Error";
            default:
                return "Unknown";
            }
        }

        std::string generate_etag(const std::filesystem::path &file_path, std::uintmax_t file_size) {
            auto last_write = std::filesystem::last_write_time(file_path);
            auto duration = last_write.time_since_epoch();
            auto hash = std::hash<std::string>{}(file_path.string()) ^ std::hash<std::uintmax_t>{}(file_size) ^
                        std::hash<long long>{}(duration.count());

            std::stringstream ss;
            ss << "\"" << std::hex << hash << "\"";
            return ss.str();
        }

        bool is_cache_valid(const CacheEntry &entry) {
            auto now = std::chrono::system_clock::now();
            return (now - entry.cached_at) < cache_ttl_;
        }

        void cleanup_cache() {
            auto now = std::chrono::system_clock::now();
            for(auto it = file_cache_.begin(); it != file_cache_.end();) {
                if(!is_cache_valid(it->second)) {
                    current_cache_size_ -= it->second.content.size();
                    it = file_cache_.erase(it);
                } else {
                    ++it;
                }
            }
        }

        void evict_cache_if_needed(size_t new_content_size) {
            while(current_cache_size_ + new_content_size > cache_max_size_ && !file_cache_.empty()) {
                auto oldest = file_cache_.begin();
                for(auto it = file_cache_.begin(); it != file_cache_.end(); ++it) {
                    if(it->second.cached_at < oldest->second.cached_at) {
                        oldest = it;
                    }
                }

                current_cache_size_ -= oldest->second.content.size();
                file_cache_.erase(oldest);
            }
        }

        std::string build_response(HttpStatus status, const std::string &content_type, const std::string &body,
                                   const std::string &etag = "") {
            std::stringstream response;
            response << "HTTP/1.1 " << static_cast<int>(status) << " " << get_status_text(status) << "\r\n";
            response << "Content-Type: " << content_type << "\r\n";
            response << "Content-Length: " << body.size() << "\r\n";
            response << "Server: ChiselHTTP/1.0\r\n";
            response << "Connection: close\r\n";

            if(!etag.empty()) {
                response << "ETag: " << etag << "\r\n";
                response << "Cache-Control: public, max-age=3600\r\n";
            } else {
                response << "Cache-Control: no-cache\r\n";
            }

            response << "\r\n";
            response << body;

            return response.str();
        }

        std::string generate_error_response(HttpStatus status, const std::string &message = "") {
            std::string body;
            std::string title = get_status_text(status);

            body = "<!DOCTYPE html>\n"
                   "<html><head><title>" +
                   std::to_string(static_cast<int>(status)) + " " + title +
                   "</title>"
                   "<style>body{font-family:Arial,sans-serif;margin:40px;}"
                   "h1{color:#d32f2f;}p{color:#666;}</style></head>"
                   "<body><h1>" +
                   std::to_string(static_cast<int>(status)) + " " + title + "</h1>";

            if(!message.empty()) {
                body += "<p>" + message + "</p>";
            } else {
                switch(status) {
                case HttpStatus::NOT_FOUND:
                    body += "<p>The requested resource was not found on this server.</p>";
                    break;
                case HttpStatus::METHOD_NOT_ALLOWED:
                    body += "<p>The requested method is not allowed for this resource.</p>";
                    break;
                case HttpStatus::BAD_REQUEST:
                    body += "<p>The request could not be understood by the server.</p>";
                    break;
                default:
                    body += "<p>An error occurred while processing your request.</p>";
                    break;
                }
            }

            body += "</body></html>";

            return build_response(status, "text/html; charset=utf-8", body);
        }

        std::string handle_request(const HttpRequest &request) {
            if(request.method != "GET") {
                std::cout << "❌ " << request.path << " - 405 Method Not Allowed (" << request.method << ")" << std::endl;
                return generate_error_response(HttpStatus::METHOD_NOT_ALLOWED);
            }

            std::string resolved_path = resolve_path(request.path);
            std::string file_path = root_dir_ + resolved_path;

            if(!std::filesystem::exists(file_path) || std::filesystem::is_directory(file_path)) {
                std::cout << "❌ " << resolved_path << " - 404 Not Found" << std::endl;
                return generate_error_response(HttpStatus::NOT_FOUND);
            }

            try {
                std::error_code ec;
                auto file_size = std::filesystem::file_size(file_path, ec);
                if(ec) {
                    std::cout << "❌ " << resolved_path << " - 500 Internal Server Error (file size)" << std::endl;
                    return generate_error_response(HttpStatus::INTERNAL_SERVER_ERROR);
                }

                std::string etag = generate_etag(file_path, file_size);

                std::string client_etag = request.get_if_none_match();
                if(!client_etag.empty() && client_etag == etag) {
                    std::cout << "📄 " << resolved_path << " - 304 Not Modified" << std::endl;
                    return build_response(HttpStatus::NOT_MODIFIED, "", "");
                }

                cleanup_cache();

                auto cache_it = file_cache_.find(resolved_path);
                if(cache_it != file_cache_.end() && is_cache_valid(cache_it->second)) {
                    const auto &entry = cache_it->second;

                    auto last_write = std::filesystem::last_write_time(file_path);
                    if(last_write <= entry.last_modified) {
                        std::cout << "📄 " << resolved_path << " - 200 OK (cached)" << std::endl;
                        return build_response(HttpStatus::OK, entry.content_type, entry.content, entry.etag);
                    } else {
                        current_cache_size_ -= entry.content.size();
                        file_cache_.erase(cache_it);
                    }
                }

                std::ifstream file(file_path, std::ios::binary);
                if(!file.good()) {
                    std::cout << "❌ " << resolved_path << " - 500 Internal Server Error (file read)" << std::endl;
                    return generate_error_response(HttpStatus::INTERNAL_SERVER_ERROR);
                }

                std::stringstream content_stream;
                content_stream << file.rdbuf();
                std::string content = content_stream.str();
                std::string content_type = get_content_type(resolved_path);

                CacheEntry cache_entry;
                cache_entry.content = content;
                cache_entry.content_type = content_type;
                cache_entry.etag = etag;
                cache_entry.last_modified = std::filesystem::last_write_time(file_path);
                cache_entry.cached_at = std::chrono::system_clock::now();
                cache_entry.file_size = file_size;

                size_t entry_size = content.size();
                if(entry_size <= cache_max_size_) {
                    evict_cache_if_needed(entry_size);
                    current_cache_size_ += entry_size;
                    file_cache_[resolved_path] = cache_entry;
                }

                std::cout << "📄 " << resolved_path << " - 200 OK" << std::endl;
                return build_response(HttpStatus::OK, content_type, content, etag);

            } catch(const std::exception &e) {
                std::cout << "❌ " << resolved_path << " - 500 Internal Server Error: " << e.what() << std::endl;
                return generate_error_response(HttpStatus::INTERNAL_SERVER_ERROR);
            }
        }
    };

} // namespace http