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
    int server_file_descriptor;
    int epoll_file_descriptor;
    uint16_t port;
    CommandDispatcher dispatcher;

    std::unordered_map<int, std::string> client_buffers;
    std::unordered_map<int, RespParser> client_parsers;

    void set_non_blocking(int file_descriptor);
    void accept_connection();
    void handle_client_data(int client_file_descriptor);
    void close_connection(int client_file_descriptor);
};

#endif
