#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "enums.hpp"

namespace app_config {


struct ClientConfig {
    std::string coordinator_host;
    int coordinator_port;
    decrypt::CipherType cipher;
    std::vector<uint8_t> encrypted_data;
};

struct CoordinatorConfig {
    int client_port;
    int worker_port;
};

struct WorkerConfig {
    // std::string listen_host;
    // int listen_port;
    std::string coordinator_host;
    int coordinator_port = 0;
};

} // namespace config