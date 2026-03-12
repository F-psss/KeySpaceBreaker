#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace app_config {


struct ClientConfig {
    std::string coordinator_host;
    int coordinator_port;
    decrypt::CipherType cipher;
    std::vector<uint8_t> encrypted_data;
};

// struct CoordinatorConfig {
//     std::string listen_host;
//     int listen_port;
//     std::vector<WorkerEndpoint> workers;
//     ScorePolicy score_policy;
// };

struct WorkerConfig {
    std::string listen_host;
    int listen_port;
    std::string worker_id;

    std::string coordinator_host;
    int coordinator_port = 0;
};

} // namespace config