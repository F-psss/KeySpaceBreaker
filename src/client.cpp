#include "client.hpp"
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include "JSON_Protocol.hpp"
#include "fstream"

asio::awaitable<void> run_client(const app_config::ClientConfig &cfg) {
    try {
        auto executor = co_await asio::this_coro::executor;
        asio::ip::tcp::socket socket(executor);
        asio::ip::tcp::resolver resolver(executor);
        auto endpoints = co_await resolver.async_resolve(
            cfg.coordinator_host, std::to_string(cfg.coordinator_port),
            asio::use_awaitable
        );

        co_await asio::async_connect(socket, endpoints, asio::use_awaitable);

        auto conn =
            std::make_shared<json_protocol::Connection>(std::move(socket));

        // ===== формируем payload =====
        auto payload = std::make_unique<json_protocol::DecryptPayload>();

        payload->set_cipher(cfg.cipher);
        payload->set_noise(cfg.noise);
        payload->set_cipher_text(cfg.encrypted_data);
        payload->set_mode(cfg.mode);
        payload->set_start_key(std::vector<uint8_t>{0});
        payload->set_end_key(std::vector<uint8_t>{25});
        payload->set_key_length(cfg.key_length);

        auto request =
            json_protocol::Message::create_decrypt_request(std::move(payload));

        // ===== отправляем =====
        co_await conn->send_message(request);
        auto start = std::chrono::steady_clock::now();

        const auto print_duration = [&](std::chrono::steady_clock::time_point end) {
            auto duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
                    .count();
            double total_seconds_double = duration / 1000.0;
            int hours = static_cast<int>(total_seconds_double / 3600);
            int minutes = static_cast<int>(
                (total_seconds_double - hours * 3600) / 60
            );
            double seconds_remain =
                total_seconds_double - hours * 3600 - minutes * 60;

            if (hours > 0) {
                std::cout << "Time:  " << hours << "h " << minutes << "min "
                          << std::fixed << std::setprecision(1) << seconds_remain
                          << "s\n";
            } else if (minutes > 0) {
                std::cout << "Time:  " << minutes << "min " << std::fixed
                          << std::setprecision(1) << seconds_remain << "s\n";
            } else {
                std::cout << "Time:  " << std::fixed << std::setprecision(2)
                          << total_seconds_double << "s\n";
            }
        };

        const auto print_progress_line =
            [](const json_protocol::StatusPayload *payload) {
                std::cout << "\r[Progress] " << payload->get_progress() << "% "
                          << "(" << payload->get_done_units() << "/"
                          << payload->get_total_units() << " units)"
                          << " | workers: " << payload->get_workers()
                          << " | in flight: " << payload->get_leased_units()
                          << " | best score: " << std::fixed
                          << std::setprecision(2) << payload->get_score()
                          << " | key: " << payload->get_key() << "   "
                          << std::flush;
            };

        bool got_final = false;
        std::string final_text;
        std::string final_key;
        double final_score = 0;
        std::size_t final_done = 0;
        std::size_t final_total = 0;

        while (true) {
            auto response = co_await conn->read_message();

            if (response.get_type() != json_protocol::MessageType::RESPONSE ||
                response.get_action() != json_protocol::Action::STATUS) {
                std::cout << "\nServer responded:\n"
                          << response.to_json().dump(4) << std::endl;
                break;
            }

            auto *payload = dynamic_cast<json_protocol::StatusPayload *>(
                response.payload.get()
            );
            if (payload == nullptr) {
                std::cout << "\nServer responded:\n"
                          << response.to_json().dump(4) << std::endl;
                break;
            }

            if (payload->is_finished()) {
                got_final = true;
                final_text = payload->get_cipher_text();
                final_key = payload->get_key();
                final_score = payload->get_score();
                final_done = payload->get_done_units();
                final_total = payload->get_total_units();
                break;
            }

            print_progress_line(payload);
        }

        if (got_final) {
            std::cout << "\n\n===== BEST RESULT =====\n";
            std::cout << "Text:  " << final_text << "\n";
            std::cout << "Key:   " << final_key << "\n";
            std::cout << "Score: " << final_score << "\n";
            std::cout << "Units: " << final_done << "/" << final_total << "\n";
            print_duration(std::chrono::steady_clock::now());
            std::cout << "=======================\n";
            if (!cfg.output_file.empty()) {
                std::ofstream out(cfg.output_file);
                if (out.is_open()) {
                    out << "Text:  " << final_text << "\n";
                    out << "Key:   " << final_key << "\n";
                    out << "Score: " << final_score << "\n";
                    std::cout << "Result saved to " << cfg.output_file << "\n";
                } else {
                    std::cerr << "Cannot open output file: " << cfg.output_file
                              << "\n";
                }
            }
        }

    } catch (const std::exception &e) {
        std::cerr << "Client error: " << e.what() << std::endl;
    }
}
