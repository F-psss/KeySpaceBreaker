#include "worker_config.hpp"
#include <stdexcept>
#include <string>
#include <vector>
#include "CLI/CLI.hpp"

app_config::WorkerConfig parse_worker_config(int argc, char **argv) {
    CLI::App app{"Worker"};

    app.set_config("--config", "", "Path to config file");

    std::vector<std::string> coordinators;
    std::string dict_path;
    std::string trigrams_path;

    app.add_option(
           "--coordinators", coordinators,
           "Coordinator addresses (host:port), comma-separated"
    )
        ->required()
        ->delimiter(',');
    app.add_option("--dict", dict_path, "path_to_dict")->required();
    app.add_option("--trigrams", trigrams_path, "path_to_trigrams")->required();
    app.parse(argc, argv);

    if (coordinators.empty()) {
        throw std::runtime_error("at least one coordinator address required");
    }

    app_config::WorkerConfig cfg;
    cfg.coordinator_addresses = coordinators;
    cfg.dict_path = dict_path;
    cfg.trigrams_path = trigrams_path;
    return cfg;
}