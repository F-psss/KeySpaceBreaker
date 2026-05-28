#include "tests.hpp"
#include "tests_config.hpp"

#include <iostream>

namespace decryptor::tests {

// Caesar: "KHOOR" = "HELLO" со сдвигом 3
// Vigenere: "RIJVS" = "HELLO" с ключом "ABC"
// Для тестов лучше брать тексты подлиннее чтобы частотный анализ работал.
// "THISISALONGTEXTWITHENOUGHCHARACTERSTORUNFREQUENCYANALYSISPROPERLY"
// со сдвигом 3 = "WKLVLVDORQJWHAWZLWKHQRXJKFKDUDFWHUVWRUXQIUHTXHQFBDQDOBVLVSURSHUOB"

#ifdef DECRYPTOR_TEST_CAESAR_BASIC

TEST(caesar, basic_decrypt) {
    Coordinator coord;
    wait_network();
    Worker worker(coord);
    wait_network();
    //TODO: переделать текст
    // "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG THE QUICK BROWN FOX" + shift 3
    const std::string ciphertext =
        "WKH TXLFN EURZQ IRA MXPSV RYHU WKH ODCB GRJ WKH TXLFN EURZQ IRA";
    const std::string expected =
        "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG THE QUICK BROWN FOX";

    auto result = Client::run_caesar(coord.client_address(), ciphertext);
    ASSERT_EQ(result.code, 0) << result.err;

    auto parsed = parse_client_output(result.out);
    EXPECT_EQ(parsed.text, expected);
    EXPECT_EQ(parsed.key, "3");
}

#endif  // DECRYPTOR_TEST_CAESAR_BASIC

#ifdef DECRYPTOR_TEST_CAESAR_DIFFERENT_SHIFTS

TEST(caesar, different_shifts) {
    Coordinator coord;
    wait_network();
    Worker worker(coord);
    wait_network();

    // Тот же текст с разными сдвигами должен расшифровываться правильно
    struct TestCase {
        std::string ciphertext;
        std::string expected_key;
    };

    std::vector<TestCase> cases = {
        // shift 1
        {"UIF RVJDL CSPXO GPY KVNQT PWFS UIF MBAZ EPH", "1"},
        // shift 13 (ROT13)
        {"GUR DHVPX OEBJA SBK WHZCF BIRE GUR YNML QBT", "13"},
        // shift 25
        {"SGD PTHBJ AQNVM ENW ITLOR NUDQ SGD KZYX CNF", "25"}
    };

    for (const auto& tc : cases) {
        auto result = Client::run_caesar(coord.client_address(), tc.ciphertext);
        ASSERT_EQ(result.code, 0) << result.err;
        auto parsed = parse_client_output(result.out);
        EXPECT_EQ(parsed.key, tc.expected_key) << "for ciphertext: " << tc.ciphertext;
    }
}

#endif  // DECRYPTOR_TEST_CAESAR_DIFFERENT_SHIFTS

#ifdef DECRYPTOR_TEST_VIGENERE_BRUTE

TEST(vigenere, brute_key_length_3) {
    Coordinator coord;
    wait_network();
    Worker worker(coord);
    wait_network();

    // "HELLO WORLD THIS IS A TEST OF VIGENERE CIPHER WITH KEY ABC"
    // c ключом "ABC"
    const std::string ciphertext =
        "HFNLP YOSLE TKJS JT B VFSU OG XJHFOFSF DKQIES YJTI LFY ACC";

    auto result = Client::run_vigenere(
        coord.client_address(), ciphertext, "brute", 3
    );
    ASSERT_EQ(result.code, 0) << result.err;

    auto parsed = parse_client_output(result.out);
    EXPECT_EQ(parsed.key, "ABC");
}

#endif  // DECRYPTOR_TEST_VIGENERE_BRUTE

#ifdef DECRYPTOR_TEST_VIGENERE_FAST

TEST(vigenere, fast_mode) {
    Coordinator coord;
    wait_network();
    Worker worker(coord);
    wait_network();

    //TODO: сгенерировать собственный текст и причём разный
    const std::string plaintext =
        "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG THIS IS A LONGER TEXT "
        "USED TO TEST THE VIGENERE CIPHER DECRYPTION IN FAST MODE WITH "
        "FREQUENCY ANALYSIS THE LONGER THE TEXT THE BETTER THE ACCURACY OF "
        "THE FREQUENCY ANALYSIS BECOMES THIS SHOULD BE ENOUGH TEXT FOR THE "
        "ALGORITHM TO WORK PROPERLY AND FIND THE CORRECT KEY WITHIN A "
        "REASONABLE TIME";

    const std::string ciphertext = plaintext;  // TODO: заменить на зашифрованный

    auto result = Client::run_vigenere(
        coord.client_address(), ciphertext, "fast", 3
    );
    ASSERT_EQ(result.code, 0) << result.err;

    auto parsed = parse_client_output(result.out);
    EXPECT_FALSE(parsed.key.empty());
    EXPECT_FALSE(parsed.text.empty());
}

#endif  // DECRYPTOR_TEST_VIGENERE_FAST

#ifdef DECRYPTOR_TEST_MULTIPLE_WORKERS

TEST(distributed, multiple_workers_same_result) {
    Coordinator coord;
    wait_network();

    Worker w1(coord);
    Worker w2(coord);
    Worker w3(coord);
    wait_network();

    const std::string ciphertext =
        "WKH TXLFN EURZQ IRA MXPSV RYHU WKH ODCB GRJ WKH TXLFN EURZQ IRA";

    auto result = Client::run_caesar(coord.client_address(), ciphertext);
    ASSERT_EQ(result.code, 0) << result.err;

    auto parsed = parse_client_output(result.out);
    EXPECT_EQ(parsed.key, "3");

    EXPECT_TRUE(w1.alive());
    EXPECT_TRUE(w2.alive());
    EXPECT_TRUE(w3.alive());
}

#endif  // DECRYPTOR_TEST_MULTIPLE_WORKERS

#ifdef DECRYPTOR_TEST_WORKER_JOINS_LATE

TEST(distributed, worker_joins_after_start) {
    Coordinator coord;
    wait_network();

    // Сначала один воркер
    Worker w1(coord);
    wait_network();

    // Запускаем клиент в отдельном потоке чтобы не блокироваться
    std::string out_holder;
    int code_holder = -1;
    std::thread client_thread([&]() {
        const std::string ciphertext =
            "WKH TXLFN EURZQ IRA MXPSV RYHU WKH ODCB GRJ";
        auto result = Client::run_caesar(coord.client_address(), ciphertext);
        code_holder = result.code;
        out_holder = result.out;
    });

    // Подключаем второй воркер с задержкой — после старта задачи
    wait_network(200);
    Worker w2(coord);

    client_thread.join();

    EXPECT_EQ(code_holder, 0);
    auto parsed = parse_client_output(out_holder);
    EXPECT_EQ(parsed.key, "3");
}

#endif  // DECRYPTOR_TEST_WORKER_JOINS_LATE

}  // namespace decryptor::tests

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    if (argc < 5) {
        std::cerr << "Usage: " << argv[0]
                  << " <coord_binary> <worker_binary> <client_binary> <dict_path>\n";
        return 1;
    }

    decryptor::tests::coord_binary = argv[1];
    decryptor::tests::worker_binary = argv[2];
    decryptor::tests::client_binary = argv[3];
    decryptor::tests::dict_path = argv[4];

    return RUN_ALL_TESTS();
}
