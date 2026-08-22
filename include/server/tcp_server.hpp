#ifndef TCP_SERVER_HPP
#define TCP_SERVER_HPP

#include "../../include/protocol/resp_parser.hpp"
#include "../../include/server/command_dispatcher.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>

class TCPServer {
  public:
    TCPServer(uint16_t port);
    ~TCPServer();
    TCPServer(const TCPServer &) = delete;
    TCPServer &operator=(const TCPServer &) = delete;
    void run();

  private:
    static constexpr size_t MAX_CLIENT_BUFFER = 1 << 20;

    int server_file_descriptor;
    int epoll_file_descriptor;
    uint16_t port;
    CommandDispatcher dispatcher;

    std::unordered_map<int, std::string> client_read_buffers;
    std::unordered_map<int, std::string> client_write_buffers;
    std::unordered_map<int, RespParser> client_parsers;

    void set_non_blocking(int file_descriptor);
    void accept_connection();
    void handle_client_data(int client_file_descriptor);
    void handle_client_writable(int client_file_descriptor);
    void queue_write(int client_file_descriptor, const std::string &data);
    void flush_write_buffer(int client_file_descriptor);
    void set_epoll_interest(int client_file_descriptor, bool want_write);
    void close_connection(int client_file_descriptor);
};

#endif
