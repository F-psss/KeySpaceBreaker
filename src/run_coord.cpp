#include <CoordServ.hpp>
#include <asio.hpp>
#include <iostream>

int main() {
    try {
        // Создаём io_context – основной цикл событий ASIO
        asio::io_context io;

        // Создаём сервер координатора
        // Первый порт – для воркеров, второй – для клиента (один клиент)
        server::CoordinatorServer server(io, 12345, 12346);

        // Запускаем акцепторы
        server.start();

        std::cout << "Coordinator server started." << std::endl;
        std::cout << "Worker port: 12345, Client port: 12346" << std::endl;

        // Запускаем цикл обработки событий (блокирует до завершения)
        io.run();

        std::cout << "Server stopped." << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
