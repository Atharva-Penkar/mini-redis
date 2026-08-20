cmake_minimum_requirement(VERSION 3.14)
project(MiniRedis CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)

include_directories(include)

add_executable(mini-redis
    src/main.cpp
    src/protocol/resp_parser.cpp
    src/storage/kv_store.cpp
    src/server/event_loop.cpp
)