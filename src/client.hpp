#pragma once
#include <asio.hpp>
#include <vector>
#include <string>
#include "../include/JSON_Protocol.hpp"

asio::awaitable<void> run_client(
    std::string host,
    int port,
    decrypt::CipherType cipher,
    std::vector<uint8_t> data
);