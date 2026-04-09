#include "worker_config.hpp"
#include "../include/CLI11-2.6.1/include/CLI/CLI.hpp"

#include <stdexcept>
#include <string>

app_config::WorkerConfig parse_worker_config(int argc, char** argv) {
    CLI::App app{"Worker"};

    app.set_config("--config", "", "Config file");

    std::string listen_host;
    int listen_port = 0;
    std::string worker_id;
    std::string coordinator_host;
    int coordinator_port = 0;

    app.add_option("--listen-host", listen_host, "Worker listen host")->required();
    app.add_option("--listen-port", listen_port, "Worker listen port")->required();
    app.add_option("--worker-id", worker_id, "Worker id")->required();

    app.add_option("--coordinator-host", coordinator_host, "Coordinator host");
    app.add_option("--coordinator-port", coordinator_port, "Coordinator port");

    app.parse(argc, argv);

    if (listen_port <= 0 || listen_port > 65535) {
        throw std::runtime_error("listen-port must be in range 1..65535");
    }

    if (coordinator_port < 0 || coordinator_port > 65535) {
        throw std::runtime_error("coordinator-port must be in range 0..65535");
    }

    app_config::WorkerConfig cfg;
    cfg.listen_host = listen_host;
    cfg.listen_port = listen_port;
    cfg.worker_id = worker_id;
    cfg.coordinator_host = coordinator_host;
    cfg.coordinator_port = coordinator_port;

    return cfg;
}