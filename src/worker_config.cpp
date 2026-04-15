#include "worker_config.hpp"
#include "CLI/CLI.hpp"

#include <stdexcept>
#include <string>

app_config::WorkerConfig parse_worker_config(int argc, char** argv) {
    CLI::App app{"Worker"};

    app.set_config("--config", "", "Path to config file");

    std::string coordinator_host = "127.0.0.1";
    int coordinator_port = 0;

    app.add_option("--coordinator-host", coordinator_host, "Coordinator host")->required();
    app.add_option("--coordinator-port", coordinator_port, "Coordinator port")->required();

    app.parse(argc, argv);

    if (coordinator_port < 0 || coordinator_port > 65535) {
        throw std::runtime_error("coordinator-port must be in range 0..65535");
    }

    app_config::WorkerConfig cfg;
    cfg.coordinator_host = coordinator_host;
    cfg.coordinator_port = coordinator_port;

    return cfg;
}