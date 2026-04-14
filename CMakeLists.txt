cmake_minimum_required(VERSION 3.10)
project(KeySpaceBracker)

set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include /usr/include/openssl)

add_library(protocol src/JSON_Protocol.cpp)
target_compile_definitions(protocol PRIVATE ASIO_STANDALONE)


add_executable(main src/main.cpp src/client.cpp)
target_compile_definitions(main PRIVATE ASIO_STANDALONE)
target_link_libraries(main PRIVATE protocol ssl crypto)

add_executable(test src/test.cpp src/Decryptor.cpp src/CaesarEncryptedMessage.cpp)
target_compile_definitions(test PRIVATE ASIO_STANDALONE)
target_link_libraries(test PRIVATE protocol ssl crypto)

add_executable(run_worker src/run_worker.cpp src/worker_config.cpp src/Decryptor.cpp src/CaesarEncryptedMessage.cpp src/StaticPolicy.cpp src/NetworkWorker.cpp)
target_compile_definitions(run_worker PRIVATE ASIO_STANDALONE)
target_link_libraries(run_worker PRIVATE protocol ssl crypto)

add_executable(run_coord src/run_coord.cpp src/Decryptor.cpp src/CaesarEncryptedMessage.cpp src/Coordinator.cpp src/StaticPolicy.cpp src/CoordServ.cpp src/ClientSession.cpp src/WorkerSession.cpp)
target_compile_definitions(run_coord PRIVATE ASIO_STANDALONE)
target_link_libraries(run_coord PRIVATE protocol ssl crypto)

