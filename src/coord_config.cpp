#include "CLI/CLI.hpp"
#include "coord_config.hpp"
#include <stdexcept>
#include <string>
app_config::CoordinatorConfig parse_coordinator_config(int argc, char **argv) {
    CLI::App app{"Coordinator config"};
    int worker_port = 0;
    int client_port = 0;
    app.set_config("--config", "", "Path to config file");
    app.add_option("--worker-port", worker_port, "Port for workers")->required();
    app.add_option("--client-port", client_port, "Port for client")->required();
    app.parse(argc, argv);
    if (client_port < 0 || client_port > 65535) {
        throw std::runtime_error("client-port must be in range 0..65535");
    }
    if (worker_port < 0 || worker_port > 65535) {
        throw std::runtime_error("coordinator-port must be in range 0..65535");
    }
    app_config::CoordinatorConfig cfg;
    cfg.worker_port = worker_port;
    cfg.client_port = client_port;
    return cfg;
}