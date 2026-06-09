#include "client.hpp"
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include "JSON_Protocol.hpp"
#include "fstream"

namespace {
json_protocol::Message build_request(const app_config::ClientConfig &cfg) {
    auto payload = std::make_unique<json_protocol::DecryptPayload>();
    payload->set_cipher(cfg.cipher);
    payload->set_noise(cfg.noise);
    payload->set_cipher_text(cfg.encrypted_data);
    payload->set_mode(cfg.mode);
    payload->set_start_key(std::vector<uint8_t>{0});
    payload->set_end_key(std::vector<uint8_t>{25});
    payload->set_key_length(cfg.key_length);
    return json_protocol::Message::create_decrypt_request(std::move(payload));
}

bool print_result(
    const json_protocol::Message &response,
    const app_config::ClientConfig &cfg,
    long long duration_ms
) {
    if (response.get_type() != json_protocol::MessageType::RESPONSE ||
        response.get_action() != json_protocol::Action::STATUS) {
        std::cout << "Server responded:\n"
                  << response.to_json().dump(4) << std::endl;
        return false;
        }
    auto *payload =
        dynamic_cast<json_protocol::StatusPayload *>(response.payload.get());
    if (payload == nullptr) {
        std::cout << "Server responded:\n"
                  << response.to_json().dump(4) << std::endl;
        return false;
    }

    std::cout << "\n===== BEST RESULT =====\n";
    std::cout << payload->get_cipher_text() << "\n";
    std::cout << "====================\n";
    std::cout << "Key:   " << payload->get_key() << "\n";
    std::cout << "Score: " << payload->get_score() << "\n";

    double total = duration_ms / 1000.0;
    int hours = static_cast<int>(total / 3600);
    int minutes = static_cast<int>((total - hours * 3600) / 60);
    double secs = total - hours * 3600 - minutes * 60;
    if (hours > 0) {
        std::cout << "Time:  " << hours << "h " << minutes << "min "
                  << std::fixed << std::setprecision(1) << secs << "s\n";
    } else if (minutes > 0) {
        std::cout << "Time:  " << minutes << "min " << std::fixed
                  << std::setprecision(1) << secs << "s\n";
    } else {
        std::cout << "Time:  " << std::fixed << std::setprecision(2) << total
                  << "s\n";
    }
    std::cout << "=======================\n";


    if (!cfg.output_file.empty()) {
        std::ofstream out(cfg.output_file);
        if (out.is_open()) {
            out << payload->get_cipher_text() << "\n";
            out << "Key:   " << payload->get_key() << "\n";
            out << "Score: " << payload->get_score() << "\n";
            std::cout << "Result saved to " << cfg.output_file << "\n";
        } else {
            std::cerr << "Cannot open output file: " << cfg.output_file << "\n";
        }
    }
    return true;
}
} // namespace

asio::awaitable<void> run_client(const app_config::ClientConfig &cfg) {
    auto executor = co_await asio::this_coro::executor;
    auto start = std::chrono::steady_clock::now();

    while (true) {
        bool sent_this_round = false;

        for (const auto &address : cfg.coordinator_addresses) {
            auto pos = address.find(':');
            if (pos == std::string::npos) {
                std::cerr << "Invalid address: " << address << std::endl;
                continue;
            }
            std::string host = address.substr(0, pos);
            std::string port_str = address.substr(pos + 1);

            try {
                asio::ip::tcp::socket socket(executor);
                asio::ip::tcp::resolver resolver(executor);
                auto endpoints = co_await resolver.async_resolve(
                    host, port_str, asio::use_awaitable
                );
                co_await asio::async_connect(
                    socket, endpoints, asio::use_awaitable
                );

                auto conn =
                    std::make_shared<json_protocol::Connection>(std::move(socket
                    ));
                std::cout << "Client connected to " << address << std::endl;

                co_await conn->send_message(build_request(cfg));
                sent_this_round = true;

                auto response = co_await conn->read_message();
                auto now = std::chrono::steady_clock::now();
                auto dur =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        now - start
                    )
                        .count();

                if (print_result(response, cfg, dur)) {
                    co_return;
                }

            } catch (const std::exception &e) {
                std::cerr << "Connection to " << address
                          << " failed/lost: " << e.what() << " — trying next"
                          << std::endl;
                continue;
            }
        }

        if (!sent_this_round) {
            std::cout << "No coordinator available, retrying in 2s..."
                      << std::endl;
        } else {
            std::cout << "Lost connection before result, retrying in 2s..."
                      << std::endl;
        }
        asio::steady_timer timer(executor);
        timer.expires_after(std::chrono::seconds(2));
        co_await timer.async_wait(asio::use_awaitable);
    }
}