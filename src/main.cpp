#include "../include/server/tcp_server.hpp"
#include <iostream>
#include <cstdint>
#include <stdexcept>

int main() {
    try {
        uint16_t port = 6379;
        std::cout << "Mini-Redis server initializing on port " << port << "...\n";
        TCPServer server(port);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal exception: " << e.what() << "\n";
        return 1;
    }
    return 0;
}