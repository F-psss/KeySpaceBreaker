#include "json.hpp"
#include <iostream>
#include <string>

using json = nlohmann::json;

json build_task_json(const std::string &cipher, const std::string &input) {
  json j;

  j["type"] = "start_task";
  j["task"]["cipher"] = cipher;
  j["task"]["cipher_params"] = {{"alphabet", "abcdefghijklmnopqrstuvwxyz"},
                                {"language", "en"}};
  j["task"]["input"] = {{"format", "text"}, {"data", input}};

  return j;
}

void run_task(const std::string &cipher, const std::string &input,
              const std::string &server) {
  std::cout << "Running task...\n";
  std::cout << "Cipher: " << cipher << "\n";
  std::cout << "Input: " << input << "\n";
  std::cout << "Server: " << server << "\n";
  std::cout << "TODO: connect to server and start cracking\n";
}

int main(int argc, char *argv[]) {
  std::string cipher;
  std::string input;
  std::string server;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--cipher" && i + 1 < argc) {
      cipher = argv[++i];
    } else if (arg == "--input" && i + 1 < argc) {
      input = argv[++i];
    } else if (arg == "--server" && i + 1 < argc) {
      server = argv[++i];
    } else if (arg == "--help") {
      std::cout << "Usage:\n"
                << "  --cipher <name>\n"
                << "  --input <text>\n"
                << "  --server <host>\n";
      return 0;
    } else {
      std::cerr << "Unknown argument: " << arg << '\n';
      return 1;
    }
  }

  if (cipher.empty() || input.empty() || server.empty()) {
    std::cerr << "Missing required arguments. Use --help\n";
    return 1;
  }

  auto request = build_task_json(cipher, input);
  std::cout << request.dump(4) << '\n';

  run_task(cipher, input, server);
  return 0;
}
