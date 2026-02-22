cmake_minimum_required(VERSION 3.10)
project(KeySpaceBracker)

set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include /usr/include/openssl)

add_library(protocol src/JSON_Protocol.cpp)
target_compile_definitions(protocol PRIVATE ASIO_STANDALONE)

add_executable(server src/server.cpp)
target_compile_definitions(server PRIVATE ASIO_STANDALONE)
target_link_libraries(server PRIVATE protocol ssl crypto)

add_executable(dem_server src/dem_server.cpp)
target_compile_definitions(dem_server PRIVATE ASIO_STANDALONE)
target_link_libraries(dem_server PRIVATE protocol ssl crypto)

add_executable(dem_client src/dem_client.cpp)
target_compile_definitions(dem_client PRIVATE ASIO_STANDALONE)
target_link_libraries(dem_client PRIVATE protocol ssl crypto)

add_executable(main src/main.cpp src/client.cpp)
target_compile_definitions(main PRIVATE ASIO_STANDALONE)
target_link_libraries(main PRIVATE protocol ssl crypto)

