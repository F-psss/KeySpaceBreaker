#include <asio.hpp>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include "CLI/CLI.hpp"
#include "../include/JSON_Protocol.hpp"
#include "client.hpp"

//
// ===================== Server =====================
//

struct Endpoint {
    std::string host;
    int port;
};

Endpoint parse_server(const std::string &addr) {
    auto pos = addr.find(':');

    if (pos == std::string::npos) {
        throw std::runtime_error("Server must be host:port");
    }

    Endpoint cfg;
    cfg.host = addr.substr(0, pos);
    cfg.port = std::stoi(addr.substr(pos + 1));

    if (cfg.port <= 0 || cfg.port > 65535) {
        throw std::runtime_error("Port must be in range 1..65535");
    }

    return cfg;
}

//
// ===================== Helpers =====================
//

std::vector<uint8_t> read_file_binary(const std::string &path) {
    std::ifstream file(path, std::ios::binary);

    if (!file) {
        throw std::runtime_error("Cannot open file: " + path);
    }

    return std::vector<uint8_t>(
        std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()
    );
}

std::vector<uint8_t> string_to_bytes(const std::string &text) {
    return std::vector<uint8_t>(text.begin(), text.end());
}

//
// ===================== Core =====================
//

app_config::ClientConfig run(int argc, char **argv) {
    CLI::App app{"Distributed brute-force decryptor"};

    //
    // ---------- config ----------
    //
    app.set_config("--config", "", "Config file");

    //
    // ---------- cipher ----------
    //
    decrypt::CipherType cipher;

    std::map<std::string, decrypt::CipherType> cipher_map{
        {"caesar", decrypt::CipherType::CAESAR},
        {"vigenere", decrypt::CipherType::VIGENERE},
        {"xor", decrypt::CipherType::XOR}
    };

    app.add_option("--cipher", cipher, "Cipher type")
        ->required()
        ->transform(CLI::CheckedTransformer(cipher_map, CLI::ignore_case));

    //
    // ---------- input ----------
    //
    std::string input_text;
    std::string input_file;

    auto input_group = app.add_option_group("Input options");

    auto input_opt =
        input_group->add_option("--input", input_text, "Encrypted text");

    auto input_file_opt = input_group->add_option(
        "--input-file", input_file, "Read encrypted data from file"
    );

    input_group->require_option(1);

    input_opt->excludes(input_file_opt);
    input_file_opt->excludes(input_opt);

    //
    // ---------- server ----------
    //
    std::string server_addr;
    std::string hostname;
    int port = 0;

    auto server_opt =
        app.add_option("--server", server_addr, "Server host:port");
    server_opt->required();
    auto host_opt = app.add_option("--hostname", hostname, "Server hostname");

    auto port_opt = app.add_option("--port", port, "Server port");

    server_opt->excludes(host_opt);
    server_opt->excludes(port_opt);

    //
    // ---------- parse ----------
    //
    app.parse(argc, argv);

    //
    // ---------- build config ----------
    //
    app_config::ClientConfig config;
    config.cipher = cipher;

    // input
    if (!input_file.empty()) {
        config.encrypted_data = read_file_binary(input_file);
    } else {
        config.encrypted_data = string_to_bytes(input_text);
    }

    // server
    if (!server_addr.empty()) {
        Endpoint ep = parse_server(server_addr);
        config.coordinator_host = ep.host;
        config.coordinator_port = ep.port;
    } else if (!hostname.empty() || port != 0) {
        if (hostname.empty() || port == 0) {
            throw std::runtime_error(
                "hostname and port must be specified together"
            );
        }

        if (port <= 0 || port > 65535) {
            throw std::runtime_error("Port must be in range 1..65535");
        }

        config.coordinator_host = hostname;
        config.coordinator_port = port;
    }

    return config;
}

//
// ===================== main =====================
//

int main(int argc, char **argv) {
    try {
        app_config::ClientConfig config = run(argc, argv);
        asio::io_context io;
        asio::co_spawn(io, run_client(config), asio::detached);
        io.run();
        return 0;
    } catch (const CLI::ParseError &e) {
        std::cerr << e.what() << '\n';
        return e.get_exit_code();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}