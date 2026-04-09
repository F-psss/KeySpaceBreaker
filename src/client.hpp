#pragma once
#include <asio.hpp>
#include <vector>
#include <string>
#include "../include/JSON_Protocol.hpp"
#include "../include/config.hpp"

asio::awaitable<void> run_client(const app_config::ClientConfig& cfg);