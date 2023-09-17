#pragma once

#include "threadpool.h"
#include "cache.h"

#include <iostream>
#include <queue>
#include <atomic>

#include <functional>
#include <unordered_map>
#include <map>
#include <queue>
#include <cstring>

#include <winSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 8192
#define CACHE_SIZE 5

inline int _send(SOCKET fd, std::string buffer) {
    return send(fd, buffer.c_str(), buffer.size(), 0);
}

inline std::string _recv(SOCKET fd) {
    char buffer[BUFFER_SIZE]{ 0 };
    if (recv(fd, buffer, BUFFER_SIZE - 1, 0) < 0)
        throw std::runtime_error("Unable to receive packet\n");
    
    return std::string(buffer);
}

typedef enum {
    GET,
    POST
}methods;

//unparsed data to be transferred from main thread to thread pool
//to be parsed and handled accordingly to reduce strain on main thread
typedef struct {
    SOCKET fd;
    std::string data;
}request_packet;

typedef struct destination {
    methods method;
    std::string path;

    // comparison between struct, mainly used by std::map
    // might cause some problems by not checking string 
    // but that would slow it down significantly
    bool operator==(const struct destination& other) const {
        return method == other.method && other.path == path;
    }
} destination;

namespace std {
    template <>
    struct hash<destination> {
        size_t operator()(const destination& dest) const {
            // Combine the hash values of 'method' and 'path'
            size_t hash = 0;
            hash = hash_combine(hash, dest.method);
            hash = hash_combine(hash, std::hash<std::string>{}(dest.path));
            return hash;
        }

        // Utility function to combine hash values
        template <typename T>
        size_t hash_combine(size_t seed, const T& val) const {
            return seed ^ (std::hash<T>{}(val)+0x9e3779b9 + (seed << 6) + (seed >> 2));
        }
    };
}

//parsed client data to be directly passed to client on a silver platter
//mostly works as a temporary buffer for the
typedef struct {
    SOCKET fd;
    destination dest;
    std::unordered_map<std::string, std::string> cookies;
} parsed_request;

typedef struct {
    int status;
    std::string body;
    std::string content_type;
    size_t length;

    int chunked;
}packet_response;

inline void set_body_content(std::string path, packet_response &content) {
    content.body = path;
    content.chunked = 0;
}

inline void set_body_media_content(std::string path, packet_response &content) {
    content.body = path;
    content.chunked = 1;
}

inline void set_content_type(std::string type, packet_response &content) {
    content.content_type = type;
}

inline void set_content_status(int code, packet_response& content) {
    content.status = code;
}

inline void display_packet(packet_response Src) {
    std::cout << "status: " << Src.status << '\n'
        << "length: " << Src.length << '\n'
        << "content-type: " << Src.content_type << '\n'
        << "body: " << Src.body << std::endl;
}

namespace http {
    class packet_parser {
    public: //variables
        std::unordered_map<int, std::string> status_codes;

    public: //public interface logic
        packet_parser() {
            status_codes[200] = "OK";
            status_codes[501] = "Not Implemented";
         }

        std::string format(packet_response Src) {
            std::string buffer;
            buffer += "HTTP/1.1 " + std::string(std::to_string(Src.status) + " " + status_codes[Src.status] + "\r\n");
            buffer += "Content-Type: " + Src.content_type+ "\r\n";
            buffer += ("Content-Length: " + std::to_string(Src.length) + "\r\n\r\n");
            buffer += Src.body;

            buffer.append("\r\n\r\n");

            return buffer;
        }

        parsed_request parse(const request_packet& request) {
            parsed_request httpRequest;
            std::string buffer(request.data);

            // find method
            int methodEnd = buffer.find(' ');
            if (methodEnd != std::string::npos) {
                std::string tmp = buffer.substr(0, methodEnd);

                // can't really make switch case with strings as that would requre an entire function call
                if (tmp == "GET")  httpRequest.dest.method = GET;
                else if (tmp == "POST") httpRequest.dest.method = POST;
            }

            // find path
            int pathStart = methodEnd + 1;
            int pathEnd = buffer.find(' ', pathStart);
            if (pathEnd != std::string::npos)
                httpRequest.dest.path = buffer.substr(pathStart, pathEnd - pathStart);

            // find cookies (if any)
            int cookieStart = buffer.find("Cookie: ");
            if (cookieStart != std::string::npos) {
                cookieStart += 8; // Skip "Cookie: "
                int cookieEnd = buffer.find("\r\n", cookieStart);
                std::string cookies = buffer.substr(cookieStart, cookieEnd - cookieStart);

                int separatorPos = cookies.find(';');
                int keyValueSeparatorPos;
                while (separatorPos != std::string::npos) {
                    std::string cookie = cookies.substr(0, separatorPos);
                    keyValueSeparatorPos = cookie.find('=');
                    if (keyValueSeparatorPos != std::string::npos) {
                        std::string key = cookie.substr(0, keyValueSeparatorPos);
                        std::string value = cookie.substr(keyValueSeparatorPos + 1);
                        httpRequest.cookies[key] = value;
                    }

                    cookies.erase(0, separatorPos + 1);
                    separatorPos = cookies.find(';');
                }

                keyValueSeparatorPos = cookies.find('=');
                if (keyValueSeparatorPos != std::string::npos) {
                    std::string key = cookies.substr(0, keyValueSeparatorPos);
                    std::string value = cookies.substr(keyValueSeparatorPos + 1);
                    httpRequest.cookies[key] = value;
                }
            }

            httpRequest.fd = request.fd;
            return httpRequest;
        }
    };

	class request_router {
	private:
        file_cache cache;
        packet_parser parser;
        std::unordered_map<destination, std::function<void(parsed_request&, packet_response&)>> routes;

    public:
        request_router(std::string cache_path) :
            cache(cache_path, CACHE_SIZE) {
        }

        inline void insert(destination path, std::function<void(parsed_request&, packet_response&)> route) {
            routes[path] = route;
        }

        void execute(request_packet packet) {
            std::cout << "execute\n";
            parsed_request request = parser.parse(packet);
            packet_response response;

            std::function<void(parsed_request&, packet_response&)> func = routes[request.dest];
            if (func == nullptr) {
                std::cout << "no route found\n" << request.dest.path << " path \n" << request.dest.method << std::endl;

                std::string buffer = "Unable to find requested path";
                response.status = 501;
                response.body = buffer;
                set_content_type("text/html", response);
            }
            else {
                func(request, response);
                std::string cache_mem = cache.fetch(response.body);
                long cache_size = cache_mem.size();

                // file transfer
                if (response.chunked) {
                    std::stringstream res;
                    res << "HTTP/1.1 200 OK\r\n"
                        << "Content-Type: " << response.content_type << "\r\n"
                        << "Transfer-Encoding: chunked\r\n"
                        << "\r\n";

                    // Send the HTTP headers
                    const std::string& responseStr = res.str();
                    send(request.fd, responseStr.c_str(), responseStr.length(), 0);

                    long bytes_sent = 0;
                    while (bytes_sent < cache_size) {
                        std::cout << "sending chunked packet\n";
                        long chunk_size = min(BUFFER_SIZE, cache_size - bytes_sent);
                        
                        std::string chunk_header; chunk_header.reserve((chunk_size % 15) + 2);
                        chunk_header.append(hexify(chunk_size));
                        chunk_header.append("\r\n");
                        send(request.fd, chunk_header.c_str(), chunk_header.size(), 0);

                        send(request.fd, cache_mem.c_str() + bytes_sent, chunk_size, 0);

                        send(request.fd, "\r\n", 2, 0);
                        
                        bytes_sent += chunk_size;
                    }

                     // final packet indicating EOF
                    send(request.fd, "0\r\n\r\n", 5, 0);
                }
                else {
                    response.body = cache_mem;
                    response.length = cache_mem.size();
                    std::string packet_buffer = parser.format(response);
                    send(request.fd, packet_buffer.c_str(), packet_buffer.size(), 0);
                }
            }

            if (closesocket(request.fd) == SOCKET_ERROR)
                std::cout << WSAGetLastError() << std::endl;
        }
    };

	class webserver {
	private:
        std::queue<request_packet> clients;
        request_router routes;
        nofetch_threadpool pool;

        SOCKET socket_fd;
        sockaddr_in server_addr;
        socklen_t server_len;

        std::thread worker;
        std::atomic<bool> flag;

    public:
        webserver(std::string cache_path, int port, int backlog, int thread_count):
        pool(thread_count),
        routes(cache_path){
            socket_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (socket_fd == SOCKET_ERROR)
                throw std::runtime_error("Unable to initialize webserver socket");

            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(port);
            server_addr.sin_addr.s_addr = INADDR_ANY;
            server_len = sizeof(server_addr);

            if (bind(socket_fd, (sockaddr*)&server_addr, server_len) == SOCKET_ERROR)
                throw std::runtime_error("Unable to bind webserver address\n");


            if (listen(socket_fd, backlog) == SOCKET_ERROR)
                throw std::runtime_error("Unable to listen on port: " + std::to_string(port));
        
            flag.store(true);
            worker = std::thread(&http::webserver::acpt_worker, this);
        }

        void shutdown() {
            flag.store(false);
            pool.shutdown();
            worker.join();
        }

        inline void get(std::string path, std::function<void(parsed_request&, packet_response&)> route) {
            destination buf;
            buf.method = GET;
            buf.path = path;

            routes.insert(buf, route);
        }

        inline void post(std::string path, std::function<void(parsed_request&, packet_response&)> route) {
            destination buf;
            buf.method = POST;
            buf.path = path;

            routes.insert(buf, route);
        }

    private:
        void acpt_worker(void) {
            while (flag.load()) {
                SOCKET socket_buf = accept(socket_fd, (sockaddr*)&server_addr, &server_len);
                if (socket_buf == INVALID_SOCKET)
                    throw std::runtime_error("Unable to accept webclient connections\n");
                std::cout << "connection received\n";

                std::string buffer = _recv(socket_buf);
                
                request_packet packet;
                packet.fd = socket_buf;
                packet.data = buffer;

                pool.exec([this, packet]() {
                    routes.execute(packet);
                 });
            }
        }
	};
};