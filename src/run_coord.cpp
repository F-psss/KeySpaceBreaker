#include <CLI/CLI.hpp>
#include <CoordServ.hpp>
#include <asio.hpp>
#include <config.hpp>
#include <iostream>
#include "coord_config.hpp"

int main(int argc, char** argv) {
    try {
        auto cfg = parse_coordinator_config(argc, argv);
        std::cout << "dskljklds";
        asio::io_context io;

        server::CoordinatorServer server(
            io,
            cfg.worker_port,
            cfg.client_port
        );
        std::cout << "sfsdfdsfs";
        server.start();

        std::cout << "Coordinator started\n";
        std::cout << "Worker port: " << cfg.worker_port << "\n";
        std::cout << "Client port: " << cfg.client_port << "\n";

        io.run();

    } catch (const CLI::ParseError& e) {
        std::cerr << e.what() << std::endl;
        return e.get_exit_code();
    } catch (const std::exception &e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
