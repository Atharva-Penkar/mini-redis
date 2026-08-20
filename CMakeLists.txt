cmake_minimum_required(VERSION 3.14)
project(MiniRedis CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(mini-redis
    src/main.cpp
    src/protocol/resp_parser.cpp
    src/storage/dict.cpp
    src/storage/kv_store.cpp
    src/server/event_loop.cpp
)

target_include_directories(mini-redis PRIVATE include)