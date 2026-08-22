#include "../../include/server/tcp_server.hpp"
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

constexpr int MAX_EVENTS = 1024;
constexpr int BUFFER_SIZE = 4096;

TCPServer::TCPServer(uint16_t port) : port(port) {
    server_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (server_file_descriptor == -1)
        throw std::runtime_error("Failed to create socket");
    int option = 1;
    setsockopt(server_file_descriptor, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    set_non_blocking(server_file_descriptor);

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_file_descriptor, reinterpret_cast<sockaddr *>(&server_address), sizeof(server_address)) == -1)
        throw std::runtime_error("Failed to bind socket");
    if (listen(server_file_descriptor, SOMAXCONN) == -1)
        throw std::runtime_error("Failed to listen on socket");

    epoll_file_descriptor = epoll_create1(0);
    if (epoll_file_descriptor == -1)
        throw std::runtime_error("Failed to create epoll instance");

    epoll_event event{};
    event.events = EPOLLIN;
    event.data.fd = server_file_descriptor;
    if (epoll_ctl(epoll_file_descriptor, EPOLL_CTL_ADD, server_file_descriptor, &event) == -1)
        throw std::runtime_error("Failed to add server socket to epoll");
}

TCPServer::~TCPServer() {
    close(server_file_descriptor);
    close(epoll_file_descriptor);
    for (const auto &pair : client_read_buffers)
        close(pair.first);
}

void TCPServer::set_non_blocking(int file_descriptor) {
    int flags = fcntl(file_descriptor, F_GETFL, 0);
    fcntl(file_descriptor, F_SETFL, flags | O_NONBLOCK);
}

void TCPServer::run() {
    std::vector<epoll_event> events(MAX_EVENTS);
    while (true) {
        int n_events = epoll_wait(epoll_file_descriptor, events.data(), MAX_EVENTS, -1);
        for (int i = 0; i < n_events; i++) {
            int fd = events[i].data.fd;
            if (fd == server_file_descriptor) {
                accept_connection();
                continue;
            }
            if (events[i].events & (EPOLLHUP | EPOLLERR)) {
                close_connection(fd);
                continue;
            }
            if (events[i].events & EPOLLOUT)
                handle_client_writable(fd);
            if (events[i].events & EPOLLIN)
                handle_client_data(fd);
        }
    }
}

void TCPServer::accept_connection() {
    sockaddr_in client_address{};
    socklen_t client_length = sizeof(client_address);
    int client_file_descriptor = accept(server_file_descriptor, reinterpret_cast<sockaddr *>(&client_address), &client_length);
    if (client_file_descriptor == -1)
        return;
    set_non_blocking(client_file_descriptor);

    epoll_event event{};
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = client_file_descriptor;
    epoll_ctl(epoll_file_descriptor, EPOLL_CTL_ADD, client_file_descriptor, &event);
    client_read_buffers[client_file_descriptor] = "";
    client_write_buffers[client_file_descriptor] = "";
    client_parsers[client_file_descriptor] = RespParser();
}

void TCPServer::set_epoll_interest(int client_file_descriptor, bool want_write) {
    epoll_event event{};
    event.events = EPOLLIN | EPOLLET | (want_write ? EPOLLOUT : 0);
    event.data.fd = client_file_descriptor;
    epoll_ctl(epoll_file_descriptor, EPOLL_CTL_MOD, client_file_descriptor, &event);
}

void TCPServer::queue_write(int client_file_descriptor, const std::string &data) {
    auto &write_buffer = client_write_buffers[client_file_descriptor];
    bool was_empty = write_buffer.empty();
    write_buffer.append(data);
    flush_write_buffer(client_file_descriptor);
    if (!client_write_buffers[client_file_descriptor].empty() && was_empty)
        set_epoll_interest(client_file_descriptor, true);
}

void TCPServer::flush_write_buffer(int client_file_descriptor) {
    auto it = client_write_buffers.find(client_file_descriptor);
    if (it == client_write_buffers.end())
        return;
    std::string &write_buffer = it->second;

    while (!write_buffer.empty()) {
        ssize_t bytes_written = write(client_file_descriptor, write_buffer.data(), write_buffer.size());
        if (bytes_written > 0) {
            write_buffer.erase(0, bytes_written);
        } else if (bytes_written == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            break;
        } else {
            close_connection(client_file_descriptor);
            return;
        }
    }

    if (write_buffer.empty())
        set_epoll_interest(client_file_descriptor, false);
}

void TCPServer::handle_client_writable(int client_file_descriptor) { flush_write_buffer(client_file_descriptor); }

void TCPServer::handle_client_data(int client_file_descriptor) {
    char buffer[BUFFER_SIZE];
    while (true) {
        ssize_t bytes_read = read(client_file_descriptor, buffer, sizeof(buffer));
        if (bytes_read > 0) {
            auto &client_buffer = client_read_buffers[client_file_descriptor];
            client_buffer.append(buffer, bytes_read);

            if (client_buffer.size() > MAX_CLIENT_BUFFER) {
                close_connection(client_file_descriptor);
                return;
            }

            auto &parser = client_parsers[client_file_descriptor];
            while (true) {
                auto parsed_object = parser.parse(client_buffer);
                if (!parsed_object)
                    break;
                std::string response = dispatcher.execute(*parsed_object);
                client_buffer.erase(0, parser.get_bytes_consumed());
                parser.reset();
                queue_write(client_file_descriptor, response);
            }
        } else if (bytes_read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            break;
        } else {
            close_connection(client_file_descriptor);
            break;
        }
    }
}

void TCPServer::close_connection(int client_file_descriptor) {
    epoll_ctl(epoll_file_descriptor, EPOLL_CTL_DEL, client_file_descriptor, nullptr);
    close(client_file_descriptor);
    client_read_buffers.erase(client_file_descriptor);
    client_write_buffers.erase(client_file_descriptor);
    client_parsers.erase(client_file_descriptor);
}
