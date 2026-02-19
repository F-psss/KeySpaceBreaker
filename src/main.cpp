#include <CLI11-2.6.1/include/CLI/CLI.hpp>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

// Тип шифра

enum class CipherType { Caesar, Vigenere, XOR };

CipherType parse_cipher(const std::string &name) {
    if (name == "caesar") {
        return CipherType::Caesar;
    }
    if (name == "vigenere") {
        return CipherType::Vigenere;
    }
    if (name == "xor") {
        return CipherType::XOR;
    }

    throw CLI::ValidationError(
        "cipher", "Invalid cipher. Allowed values: caesar, vigenere, xor"
    );
}

// сервер

struct ServerConfig {
    std::string host;
    int port;
};

ServerConfig parse_server(const std::string &addr) {
    auto pos = addr.find(':');
    if (pos == std::string::npos) {
        throw std::runtime_error("Server must be in format host:port");
    }

    ServerConfig cfg;
    cfg.host = addr.substr(0, pos);

    try {
        cfg.port = std::stoi(addr.substr(pos + 1));
    } catch (...) {
        throw std::runtime_error("Port must be a valid integer");
    }

    if (cfg.port <= 0 || cfg.port > 65535) {
        throw std::runtime_error("Port must be between 1 and 65535");
    }

    return cfg;
}

// настройки приложения

struct AppConfig {
    CipherType cipher;
    std::string encrypted_text;
    std::optional<ServerConfig> server;
};

int main(int argc, char **argv) {
    CLI::App app{"Distributed brute-force decryptor"};

    std::string cipher_name;
    std::string input_text;
    std::string input_file;

    std::string server_addr;
    std::string hostname;
    int port = 0;

    std::string config_file;
    app.add_option("--config", config_file, "Path to config file");

    app.add_option(
           "--cipher", cipher_name, "Cipher type: caesar | vigenere | xor"
    )
        ->required();

    auto input_group = app.add_option_group("Input options");

    input_group->add_option("--input", input_text, "Encrypted text as string");

    input_group->add_option(
        "--input_file", input_file, "Path to file with encrypted text"
    );

    input_group->require_option(1);

    app.add_option(
        "--server", server_addr, "Server address in format host:port"
    );

    app.add_option("--hostname", hostname, "Server hostname");

    app.add_option("--port", port, "Server port");

    CLI11_PARSE(app, argc, argv);

    AppConfig config;

    try {
        config.cipher = parse_cipher(cipher_name);
    } catch (const CLI::ValidationError &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    if (!input_file.empty()) {
        std::ifstream file(input_file);
        if (!file) {
            std::cerr << "Cannot open file: " << input_file << std::endl;
            return 1;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        config.encrypted_text = buffer.str();
    } else {
        config.encrypted_text = input_text;
    }

    try {
        if (!server_addr.empty()) {
            config.server = parse_server(server_addr);
        } else if (!hostname.empty() && port != 0) {
            if (port <= 0 || port > 65535) {
                throw std::runtime_error("Port must be between 1 and 65535");
            }

            config.server = ServerConfig{hostname, port};
        } else if (!hostname.empty() || port != 0) {
            throw std::runtime_error(
                "If using hostname/port, both must be provided"
            );
        }
    } catch (const std::exception &e) {
        std::cerr << "Server config error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Configuration loaded successfully\n";

    std::cout << "Cipher: " << cipher_name << "\n";
    std::cout << "Encrypted text length: " << config.encrypted_text.size()
              << "\n";

    if (config.server.has_value()) {
        std::cout << "Server: " << config.server->host << ":"
                  << config.server->port << "\n";
    } else {
        std::cout << "Server: not specified\n";
    }

    return 0;
}
