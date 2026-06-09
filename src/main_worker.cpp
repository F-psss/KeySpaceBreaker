#include <asio.hpp>
#include <iostream>
#include "CLI/CLI.hpp"
#include "NetworkWorker.hpp"
#include "config.hpp"
#include "worker_config.hpp"

int main(int argc, char **argv) {
    try {
        auto cfg = parse_worker_config(argc, argv);

        asio::io_context io;

        Worker worker(io, cfg.coordinator_addresses, cfg.dict_path);
        worker.start();

        io.run();
        return 0;
    } catch (const CLI::ParseError &e) {
        std::cerr << e.what() << std::endl;
        return e.get_exit_code();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}