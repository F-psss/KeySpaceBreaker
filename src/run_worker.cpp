#include <NetworkWorker.hpp>
#include <iostream>
#include <memory>
#include <random>
#include <string>

int main() {
    asio::io_context io_context;
    Worker worker(io_context, "127.0.0.1", 12345);
    worker.start();
    io_context.run();

    return 0;
}
