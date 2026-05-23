cmake_minimum_required(VERSION 3.10)
project(KeySpaceBreaker)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Пути для Homebrew на Apple Silicon
set(BREW_PREFIX "/opt/homebrew")
set(OPENSSL_ROOT "${BREW_PREFIX}/opt/openssl")

# Включение директорий
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${BREW_PREFIX}/include
        ${OPENSSL_ROOT}/include
)

# Поиск библиотек
link_directories(
        ${BREW_PREFIX}/lib
        ${OPENSSL_ROOT}/lib
)

add_library(protocol src/JSON_Protocol.cpp)
target_compile_definitions(protocol PRIVATE ASIO_STANDALONE)

add_executable(main src/main.cpp src/client.cpp)
target_compile_definitions(main PRIVATE ASIO_STANDALONE)
target_link_libraries(main PRIVATE protocol ssl crypto)

add_executable(run_worker
        src/main_worker.cpp
        src/coord_config.cpp
        src/worker_config.cpp
        src/Decryptor.cpp
        src/Dictionary.cpp
        src/VigenereEncryptedMessage.cpp
        src/CaesarEncryptedMessage.cpp
        src/StaticPolicy.cpp
        src/NetworkWorker.cpp
)
target_compile_definitions(run_worker PRIVATE ASIO_STANDALONE)
target_link_libraries(run_worker PRIVATE protocol ssl crypto)

add_executable(run_coord
        src/PeerSession.cpp
        src/run_coord.cpp
        src/coord_config.cpp
        src/Decryptor.cpp
        src/Dictionary.cpp
        src/VigenereEncryptedMessage.cpp
        src/CaesarEncryptedMessage.cpp
        src/Coordinator.cpp
        src/StaticPolicy.cpp
        src/CoordServ.cpp
        src/ClientSession.cpp
        src/WorkerSession.cpp
        src/KasiskiAnalyzer.cpp
)
target_compile_definitions(run_coord PRIVATE ASIO_STANDALONE)
target_link_libraries(run_coord PRIVATE protocol ssl crypto)