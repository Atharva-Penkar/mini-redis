cmake_minimum_required(VERSION 3.14)
project(MiniRedis CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(mini-redis-core STATIC
    src/protocol/resp_parser.cpp 
    src/protocol/resp_serializer.cpp
    src/storage/dict.cpp 
    src/server/command_dispatcher.cpp
    src/server/tcp_server.cpp 
)

add_executable(mini-redis src/main.cpp)
target_link_libraries(mini-redis PRIVATE mini-redis-core)

add_executable(runTests tests/run_tests.cpp)
target_link_libraries(runTests PRIVATE mini-redis-core)