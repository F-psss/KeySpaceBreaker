cmake_minimum_required(VERSION 3.10)
project(KeySpaceBracker)

set(CMAKE_CXX_STANDARD 20)

add_executable(server server.cpp)
target_compile_definitions(server PRIVATE ASIO_STANDALONE)

add_executable(client client.cpp)
target_compile_definitions(client PRIVATE ASIO_STANDALONE)
