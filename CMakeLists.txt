cmake_minimum_required(VERSION 3.10)
project(KeySpaceBracker)

set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(protocol src/JSON_Protocol.cpp)
target_compile_definitions(protocol PRIVATE ASIO_STANDALONE)

add_executable(server src/server.cpp)
target_compile_definitions(server PRIVATE ASIO_STANDALONE)
target_link_libraries(server PRIVATE protocol)

add_executable(client src/client.cpp)
target_compile_definitions(client PRIVATE ASIO_STANDALONE)
target_link_libraries(client PRIVATE protocol)

