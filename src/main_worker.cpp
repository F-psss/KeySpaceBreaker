#include <asio.hpp>
#include <iostream>
#include "config.hpp"
#include "CLI/CLI.hpp"
#include "worker_config.hpp"
#include "NetworkWorker.hpp"

int main(int argc, char** argv) {
    try {
        auto cfg = parse_worker_config(argc, argv);

        asio::io_context io;

        Worker worker(io, cfg.coordinator_host, cfg.coordinator_port);
        worker.start();

        io.run();
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
}