#include "CLI/CLI.hpp"
#include "coord_config.hpp"
#include <stdexcept>
#include <string>
app_config::CoordinatorConfig parse_coordinator_config(int argc, char **argv) {
    CLI::App app{"Coordinator config"};
    int worker_port = 0;
    int client_port = 0;
    int id = 0;
    int peer_port = 0;
    std::vector<std::string> peer_addresses;
    std::string checkpoint_path = "coordinator_checkpoint.json";

    app.set_config("--config", "", "Path to config file");

    app.add_option("--worker-port", worker_port, "Port for workers")->required();
    app.add_option("--client-port", client_port, "Port for client")->required();

    app.add_option("--id", id, "Coordinator ID (unique, lower = higher priority)")
        ->required();

    app.add_option("--peer-port", peer_port, "Port for peer-to-peer connections")
        ->required();

    app.add_option(
           "--peers", peer_addresses,
           "Addresses of other coordinators (host:peer_port), comma-separated"
       )
        ->delimiter(',');

    app.add_option("--checkpoint", checkpoint_path, "Path to checkpoint file");

    app.parse(argc, argv);
    if (client_port < 0 || client_port > 65535) {
        throw std::runtime_error("client-port must be in range 0..65535");
    }
    if (worker_port < 0 || worker_port > 65535) {
        throw std::runtime_error("coordinator-port must be in range 0..65535");
    }

    if (peer_port < 0 || peer_port > 65535) {
        throw std::runtime_error("peer-port must be in range 0..65535");
    }
    if (id <= 0) {
        throw std::runtime_error("id must be positive");
    }

    app_config::CoordinatorConfig cfg;
    cfg.worker_port = worker_port;
    cfg.client_port = client_port;
    cfg.id = id;
    cfg.peer_port = peer_port;
    cfg.peer_addresses = peer_addresses;
    cfg.checkpoint_path = checkpoint_path;
    return cfg;
}