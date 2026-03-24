#include <NetworkWorker.hpp>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include "worker_config.hpp"

int main(int argc, char** argv) {
    try {
        auto cfg = parse_worker_config(argc, argv);

        std::cout << "Worker config loaded:\n";
        std::cout << "  worker_id: " << cfg.worker_id << "\n";
        std::cout << "  listen: " << cfg.listen_host << ":" << cfg.listen_port << "\n";

        if (!cfg.coordinator_host.empty()) {
            std::cout << "  coordinator: "
                      << cfg.coordinator_host << ":" << cfg.coordinator_port << "\n";
        }
    asio::io_context io_context;
    Worker worker(io_context, "127.0.0.1", 12345);
    worker.start();
    io_context.run();
        return 0;
    }
    catch (const CLI::ParseError& e) {
        std::cerr << e.what() << std::endl;
        return e.get_exit_code();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}