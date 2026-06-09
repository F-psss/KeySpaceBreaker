#include "tests.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <iostream>

namespace decryptor::tests {

using json = nlohmann::json;

// Один тестовый кейс, загруженный из файлов
struct DataCase {
    std::string name;          // префикс, например "t-02-caesar-shift3"
    std::string cipher;        // "caesar" | "vigenere"
    std::string mode;          // "brute" | "fast"
    int key_length = 1;
    double noise = 0.5;
    std::string expected_key;
    std::string ciphertext;    // содержимое -in.txt
    std::string expected_text; // содержимое -sol.txt
};

// Для красивого имени теста в выводе gtest
std::string sanitize(const std::string& s) {
    std::string r;
    for (char c : s) {
        r += (std::isalnum(static_cast<unsigned char>(c)) != 0) ? c : '_';
    }
    return r;
}

// Сканируем TEST_DATA_DIR, собираем все кейсы по наличию *-meta.json
std::vector<DataCase> load_cases() {
    std::vector<DataCase> cases;
    fs::path dir = TEST_DATA_DIR;

    if (!fs::exists(dir)) {
        return cases;
    }

    for (const auto& entry : fs::directory_iterator(dir)) {
        const std::string fname = entry.path().filename().string();
        const std::string suffix = "-meta.json";
        if (fname.size() <= suffix.size() ||
            fname.compare(fname.size() - suffix.size(), suffix.size(), suffix) != 0) {
            continue;
        }

        std::string name = fname.substr(0, fname.size() - suffix.size());

        json meta = json::parse(read_file(entry.path()), nullptr, false);
        if (meta.is_discarded()) {
            std::cerr << "Bad meta.json: " << fname << "\n";
            continue;
        }

        DataCase dc;
        dc.name = name;
        dc.cipher = meta.value("cipher", "caesar");
        dc.mode = meta.value("mode", "brute");
        dc.key_length = meta.value("key_length", 1);
        dc.noise = meta.value("noise", 0.5);
        dc.expected_key = meta.value("expected_key", "");
        dc.ciphertext = read_file(dir / (name + "-in.txt"));
        dc.expected_text = read_file(dir / (name + "-sol.txt"));

        cases.push_back(std::move(dc));
    }

    std::sort(cases.begin(), cases.end(), [](const DataCase& a, const DataCase& b) {
        return a.name < b.name;
    });
    return cases;
}

class DecryptData : public ::testing::TestWithParam<DataCase> {};

TEST_P(DecryptData, decrypts_correctly) {
    const DataCase& tc = GetParam();

    Coordinator coord;
    wait_network();
    Worker worker(coord);
    wait_network();

    fs::path in_file = fs::path(TEST_DATA_DIR) / (tc.name + "-in.txt");

    CommandResult result;
    if (tc.cipher == "caesar") {
        result = Client::run({
            "--cipher", "caesar",
            "--input-file", in_file.string(),
            "--servers", coord.client_address(),
            "--noise", std::to_string(tc.noise)
        });
    } else {
        result = Client::run({
            "--cipher", "vigenere",
            "--mode", tc.mode,
            "--input-file", in_file.string(),
            "--servers", coord.client_address(),
            "--key-length", std::to_string(tc.key_length),
            "--noise", std::to_string(tc.noise)
        });
    }
    ASSERT_EQ(result.code, 0) << result.err;

    auto parsed = parse_client_output(result.out);
    fs::path out_file = fs::path(TEST_DATA_DIR) / (tc.name + "-out.txt");
    std::ofstream out(out_file);
    out << parsed.text;
    out.close();

    bool text_ok = (parsed.text == tc.expected_text);
    bool key_ok = tc.expected_key.empty() || (parsed.key == tc.expected_key);

    EXPECT_TRUE(text_ok)
        << "case: " << tc.name
        << "\n  expected key: " << tc.expected_key
        << "\n  got key:      " << parsed.key
        << "\n  diff: compare " << out_file.string()
        << " with " << (tc.name + "-sol.txt");
    EXPECT_TRUE(key_ok) << "case: " << tc.name
        << " expected " << tc.expected_key << " got " << parsed.key;

    if (!tc.expected_key.empty()) {
        EXPECT_EQ(parsed.key, tc.expected_key) << "case: " << tc.name;
    }
}

INSTANTIATE_TEST_SUITE_P(
    FromFiles,
    DecryptData,
    ::testing::ValuesIn(load_cases()),
    [](const ::testing::TestParamInfo<DataCase>& info) {
        return sanitize(info.param.name);
    }
);

}  // namespace decryptor::tests
