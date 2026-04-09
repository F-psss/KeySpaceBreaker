#include <asio.hpp>
#include <iostream>
#include "../include/config.hpp"

app_config::WorkerConfig parse_worker_config(int argc, char** argv);
asio::awaitable<void> run_worker(const app_config::WorkerConfig& cfg);

int main(int argc, char** argv) {
    try {
        auto cfg = parse_worker_config(argc, argv);

        asio::io_context io;
        asio::co_spawn(io, run_worker(cfg), asio::detached);
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