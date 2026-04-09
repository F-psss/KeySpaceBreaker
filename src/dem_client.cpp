#include <JSON_Protocol.hpp>
#include <iostream>
#include <memory>
#include <random>
#include <string>

std::vector<uint8_t> data(1000);

asio::awaitable<void> read_from_server(
    std::shared_ptr<json_protocol::Connection> conn
) {
    while (conn->is_open()) {
        json_protocol::Message response = co_await conn->read_message();
        json_protocol::Message request;

        // Используем фабричный метод для создания ответа
        if (response.get_action() == json_protocol::Action::PING) {
            // Получаем доступ к payload
            auto *payload = dynamic_cast<json_protocol::PingPayload *>(
                response.payload.get()
            );
            if (payload == nullptr) {
                std::cerr << "Invalid payload for FIBONACCI action"
                          << std::endl;
                continue;
            }

            std::cout << "fib: " << payload->get_code() << std::endl;

            // Вычисляем следующее число
            int next_fib = payload->get_code() + 1;

            auto new_payload = std::make_unique<json_protocol::PingPayload>();
            new_payload->set_code(next_fib);
            request = json_protocol::Message::create_ping_request(
                std::move(new_payload)
            );
        }

        if (response.get_action() == json_protocol::Action::DECRYPT) {
            // Получаем доступ к payload
            auto *payload = dynamic_cast<json_protocol::DecryptPayload *>(
                response.payload.get()
            );
            if (payload == nullptr) {
                std::cerr << "Invalid payload for FIBONACCI action"
                          << std::endl;
                continue;
            }

            std::cout << "new: "
                      << payload->get_cipher_text()[payload->get_start_key()[0]]
                      << std::endl;

            auto new_payload =
                std::make_unique<json_protocol::DecryptPayload>();
            new_payload->set_cipher(decrypt::CipherType::XOR);
            new_payload->set_cipher_text(payload->get_cipher_text());
            new_payload->set_start_key(std::vector<uint8_t>{
                payload->get_start_key()[0]++});
            new_payload->set_end_key(std::vector<uint8_t>{255});
            request = json_protocol::Message::create_decrypt_request(
                std::move(new_payload)
            );
        }
        std::cout << "Ожидание 1 секунду..." << std::endl;
        asio::steady_timer timer(co_await asio::this_coro::executor);
        timer.expires_after(std::chrono::seconds(1));
        co_await timer.async_wait(asio::use_awaitable);

        co_await conn->send_message(request);
    }
}

// Корутина для чтения с stdin, без неё не выводило, крутилось в цикле ввода
asio::awaitable<void> read_from_stdin(
    std::shared_ptr<json_protocol::Connection> conn
) {
    auto executor = co_await asio::this_coro::executor;
    asio::posix::stream_descriptor stdin_stream(executor, ::dup(STDIN_FILENO));

    asio::streambuf buffer;
    std::string last_command;

    try {
        while (conn->is_open()) {
            std::cout << "> " << std::flush;

            // Асинхронно читаем из stdin
            size_t n = co_await async_read_until(
                stdin_stream, buffer, '\n', asio::use_awaitable
            );

            // Извлекаем строку
            std::istream is(&buffer);
            std::string line;
            std::getline(is, line);

            // Убираем пробелы
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);

            if (line.empty()) {
                continue;
            }

            // Если последняя команда была "start", обрабатываем числа
            if (last_command == "start") {
                std::istringstream iss(line);
                int a{};
                int b{};
                if (iss >> a >> b) {
                    auto new_payload =
                        std::make_unique<json_protocol::DecryptPayload>();
                    new_payload->set_cipher(decrypt::CipherType::XOR);
                    new_payload->set_cipher_text(data);
                    new_payload->set_start_key(std::vector<uint8_t>{0});
                    new_payload->set_end_key(std::vector<uint8_t>{255});

                    auto request =
                        json_protocol::Message::create_decrypt_request(
                            std::move(new_payload)
                        );

                    // ✅ 4. Проверяем JSON перед отправкой
                    json j = request.to_json();
                    std::cout << "📤 Отправляемый JSON: " << j.dump(2)
                              << std::endl;
                    co_await conn->send_message(request);
                } else {
                    std::cout << "Ошибка: введите два числа" << std::endl;
                }
                last_command.clear();
                continue;
            }

            // Обрабатываем команды
            if (line == "quit") {
                std::cout << "Exiting..." << std::endl;
                conn->close();
                break;
            } else if (line == "start") {
                std::cout << "Введите два числа:" << std::endl;
                last_command = "start";
            }

            // Очищаем буфер
            buffer.consume(buffer.size());
        }
    } catch (const std::exception &e) {
        std::cout << "Input error: " << e.what() << std::endl;
    }
}

// Основная корутина клиента
asio::awaitable<void> client() {
    try {
        auto executor = co_await asio::this_coro::executor;
        asio::ip::tcp::socket socket(executor);

        std::cout << "Connecting..." << std::endl;

        co_await socket.async_connect(
            asio::ip::tcp::endpoint(asio::ip::make_address("127.0.0.1"), 12345),
            asio::use_awaitable
        );

        std::cout << "Connected! Type messages below." << std::endl;
        auto conn =
            std::make_shared<json_protocol::Connection>(std::move(socket));
        asio::co_spawn(executor, read_from_server(conn), asio::detached);
        asio::co_spawn(executor, read_from_stdin(conn), asio::detached);

        while (socket.is_open()) {
            asio::steady_timer timer(executor);
            timer.expires_after(std::chrono::seconds(1));
            co_await timer.async_wait(asio::use_awaitable);
        }

    } catch (const std::exception &e) {
        std::cerr << "Client error: " << e.what() << std::endl;
    }
}

int main() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 255);

    for (int i = 0; i < 1000; i++) {
        data[i] = static_cast<uint8_t>(dist(gen));
    }
    asio::io_context io_context;
    asio::co_spawn(io_context, client(), asio::detached);
    io_context.run();

    return 0;
}
