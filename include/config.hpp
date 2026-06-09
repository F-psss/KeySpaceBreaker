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
    decrypt::VigenereMode mode;
    int key_length;
    double noise;
    std::vector<uint8_t> encrypted_data;
    std::string output_file;
};

struct CoordinatorConfig {
    int client_port;
    int worker_port;
    int id = 0;                                 
    int peer_port = 0;                         
    std::vector<std::string> peer_addresses;     

    std::string checkpoint_path = "coordinator_checkpoint.json";
};

struct WorkerConfig {
    std::vector<std::string> coordinator_addresses; 
    std::string dict_path;
};

} // namespace config