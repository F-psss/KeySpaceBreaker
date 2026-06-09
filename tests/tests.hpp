#ifndef DECRYPTOR_TESTS_HPP
#define DECRYPTOR_TESTS_HPP

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace decryptor::tests {

namespace fs = std::filesystem;

// Пути до бинарников, задаются из main() через argv
inline fs::path coord_binary;
inline fs::path worker_binary;
inline fs::path client_binary;
inline fs::path dict_path;
inline fs::path trigrams_path;

struct CommandResult {
    int code = -1;
    std::string out;
    std::string err;
};

// Распарсенный результат клиента
struct ClientResult {
    std::string key;
    std::string text;
    double score = 0.0;
};

inline std::string shell_quote(const std::string& value) {
    std::string result = "'";
    for (char c : value) {
        if (c == '\'') {
            result += "'\\''";
        } else {
            result += c;
        }
    }
    result += "'";
    return result;
}

inline std::string read_file(const fs::path& path) {
    std::ifstream input(path);
    std::stringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

inline fs::path temp_path() {
    const auto now =
        std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return fs::temp_directory_path() /
           ("decryptor_test_" + std::to_string(now));
}

// Свободный порт для каждого теста — простой счётчик
inline int next_port() {
    static std::atomic<int> port{
        25000 +
        static_cast<int>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count() %
            10000
        )};
    return port.fetch_add(1);
}

inline void wait_network(int ms = 500) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// Парсинг вывода клиента (формат который сейчас печатает client.cpp)
inline ClientResult parse_client_output(const std::string& out) {
    ClientResult result;
    std::istringstream iss(out);
    std::string line;
    bool in_text = false;
    std::string text;

    auto strip = [](std::string s) {
        size_t start = s.find_first_not_of(" \t");
        if (start == std::string::npos) return std::string{};
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    };

    while (std::getline(iss, line)) {
        // убираем возможный \r в конце (на случай CRLF)
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line == "===== BEST RESULT =====") {
            in_text = true;
            continue;
        }
        if (line == "====================") {
            in_text = false;
            continue;
        }
        if (in_text) {
            if (!text.empty()) {
                text += "\n";
            }
            text += line;
            continue;
        }
        if (line.find("Key:") == 0) {
            result.key = strip(line.substr(4));
        } else if (line.find("Score:") == 0) {
            try {
                result.score = std::stod(strip(line.substr(6)));
            } catch (...) {
            }
        }
    }
    result.text = text;
    return result;
}

// Долгоживущий процесс (координатор, воркер) — запускается через fork+exec,
// убивается в деструкторе
class BackgroundProcess {
public:
    BackgroundProcess(
        const fs::path& binary,
        const std::vector<std::string>& args,
        const fs::path& log_dir
    )
        : log_dir_(log_dir) {
        fs::create_directories(log_dir_);
        fs::path out_path = log_dir_ / "stdout.log";
        fs::path err_path = log_dir_ / "stderr.log";

        pid_ = fork();
        if (pid_ == 0) {
            // Перенаправляем вывод в файлы
            freopen(out_path.string().c_str(), "w", stdout);
            freopen(err_path.string().c_str(), "w", stderr);

            std::vector<char*> argv;
            argv.push_back(const_cast<char*>(binary.c_str()));
            for (const auto& a : args) {
                argv.push_back(const_cast<char*>(a.c_str()));
            }
            argv.push_back(nullptr);

            execv(binary.c_str(), argv.data());
            _exit(127); // execv не должен возвращаться
        }
    }

    ~BackgroundProcess() {
        if (pid_ > 0) {
            kill(pid_, SIGTERM);
            int status = 0;
            // Ждём не вечно — даём 1 сек, потом SIGKILL
            for (int i = 0; i < 10; ++i) {
                pid_t r = waitpid(pid_, &status, WNOHANG);
                if (r != 0) return;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            kill(pid_, SIGKILL);
            waitpid(pid_, &status, 0);
        }
    }

    BackgroundProcess(const BackgroundProcess&) = delete;
    BackgroundProcess& operator=(const BackgroundProcess&) = delete;

    pid_t pid() const { return pid_; }
    bool alive() const {
        if (pid_ <= 0) return false;
        int status = 0;
        pid_t r = waitpid(pid_, &status, WNOHANG);
        return r == 0;
    }

private:
    pid_t pid_ = -1;
    fs::path log_dir_;
};

class Coordinator {
public:
    Coordinator(int id = 1)
        : temp_dir_(temp_path()),
          worker_port_(next_port()),
          client_port_(next_port()),
          peer_port_(next_port()),
          id_(id) {
        fs::create_directories(temp_dir_);
        fs::path checkpoint = temp_dir_ / "checkpoint.json";

        process_ = std::make_unique<BackgroundProcess>(
            coord_binary,
            std::vector<std::string>{
                "--worker-port", std::to_string(worker_port_),
                "--client-port", std::to_string(client_port_),
                "--peer-port", std::to_string(peer_port_),
                "--id", std::to_string(id_),
                "--checkpoint", checkpoint.string()
            },
            temp_dir_ / "coord_logs"
        );
    }

    ~Coordinator() {
        process_.reset();
        std::error_code ec;
        fs::remove_all(temp_dir_, ec);
    }

    int worker_port() const { return worker_port_; }
    int client_port() const { return client_port_; }
    int peer_port() const { return peer_port_; }
    bool alive() const { return process_ && process_->alive(); }

    std::string client_address() const {
        return "127.0.0.1:" + std::to_string(client_port_);
    }

private:
    fs::path temp_dir_;
    int worker_port_;
    int client_port_;
    int peer_port_;
    int id_;
    std::unique_ptr<BackgroundProcess> process_;
};

class Worker {
public:
    Worker(const Coordinator& coord)
        : temp_dir_(temp_path()) {
        fs::create_directories(temp_dir_);
        process_ = std::make_unique<BackgroundProcess>(
            worker_binary,
            std::vector<std::string>{
                "--coordinators", "127.0.0.1:" + std::to_string(coord.worker_port()),
                "--dict", dict_path.string(),
                "--trigrams", trigrams_path.string()
            },
            temp_dir_ / "worker_logs"
        );
    }

    ~Worker() {
        process_.reset();
        std::error_code ec;
        fs::remove_all(temp_dir_, ec);
    }

    bool alive() const { return process_ && process_->alive(); }

private:
    fs::path temp_dir_;
    std::unique_ptr<BackgroundProcess> process_;
};

// Клиент — короткоживущий, запускаем синхронно через std::system
class Client {
public:
    static CommandResult run(const std::vector<std::string>& args) {
        fs::path tmp = temp_path();
        fs::create_directories(tmp);
        fs::path out_path = tmp / "out.txt";
        fs::path err_path = tmp / "err.txt";

        std::string command = shell_quote(client_binary.string());
        for (const auto& a : args) {
            command += " " + shell_quote(a);
        }
        command += " > " + shell_quote(out_path.string()) +
                   " 2> " + shell_quote(err_path.string());

        CommandResult result;
        result.code = std::system(command.c_str());
        result.out = read_file(out_path);
        result.err = read_file(err_path);

        std::error_code ec;
        fs::remove_all(tmp, ec);
        return result;
    }

    static CommandResult run_caesar(
        const std::string& server,
        const std::string& ciphertext,
        double noise = 0.5
    ) {
        return run({
            "--cipher", "caesar",
            "--input", ciphertext,
            "--servers", server,
            "--noise", std::to_string(noise)
        });
    }

    static CommandResult run_vigenere(
        const std::string& server,
        const std::string& ciphertext,
        const std::string& mode,
        int key_length,
        double noise = 0.0
    ) {
        return run({
            "--cipher", "vigenere",
            "--mode", mode,
            "--input", ciphertext,
            "--servers", server,
            "--key-length", std::to_string(key_length),
            "--noise", std::to_string(noise)
        });
    }
};

}  // namespace decryptor::tests

#endif  // DECRYPTOR_TESTS_HPP
