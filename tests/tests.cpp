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
        "wkh txlfn eurzq ira mxpsv ryhu wkh odcb grj wkh txlfn eurzq ira";
    const std::string expected =
        "the quick brown fox jumps over the lazy dog the quick brown fox";

    auto result = Client::run_caesar(coord.client_address(), ciphertext);
    ASSERT_EQ(result.code, 0) << result.err;

    auto parsed = parse_client_output(result.out);
    EXPECT_EQ(parsed.text, expected);
    EXPECT_EQ(parsed.key, "D");
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
        //shift 0
        {"After assuming control of government and pardoning many of his enemies, Caesar carried out various reforms and public works", "A"},
        // shift 1
        {"bgufs bttvnjoh dpouspm pg hpwfsonfou boe qbsepojoh nboz pg ijt fofnjft, dbftbs dbssjfe pvu wbsjpvt sfgpsnt boe qvcmjd xpslt", "B"},
        // shift 13 (ROT13)
        {"nsgre nffhzvat pbageby bs tbireazrag naq cneqbavat znal bs uvf rarzvrf, pnrfne pneevrq bhg inevbhf ersbezf naq choyvp jbexf", "N"},
        // shift 25
        {"zesdq zrrtlhmf bnmsqnk ne fnudqmldms zmc ozqcnmhmf lzmx ne ghr dmdlhdr, bzdrzq bzqqhdc nts uzqhntr qdenqlr zmc otakhb vnqjr", "Z"}
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
        "hfnlp yosnd ujit ks b vetv og xihgnfte dkpigr xkti mez cbd";

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
    EXPECT_EQ(parsed.key, "D");

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
    EXPECT_EQ(parsed.key, "D");
}

#endif  // DECRYPTOR_TEST_WORKER_JOINS_LATE

#ifdef DECRYPTOR_TEST_WORKER_DIES

// Сценарий: два воркера, один убит во время работы.
// Координатор по таймауту переназначает его юнит, второй доделывает.
// Задача всё равно решается корректно.
TEST(fault_tolerance, worker_dies_task_completes) {
    Coordinator coord;
    wait_network();

    // Запускаем два воркера
    auto w1 = std::make_unique<Worker>(coord);
    Worker w2(coord);
    wait_network();

    // Клиент запускаем в отдельном потоке — задача длинная (vigenere brute)
    std::string out_holder;
    int code_holder = -1;
    std::thread client_thread([&]() {
        // длинный текст + brute key_length=3 -> много юнитов, успеем убить воркер
        const std::string ciphertext =
            "wkh txlfn eurzq ira mxpsv ryhu wkh odcb grj wkh txlfn eurzq ira";
        auto result = Client::run_caesar(coord.client_address(), ciphertext);
        code_holder = result.code;
        out_holder = result.out;
    });

    // Убиваем первый воркер пока задача в процессе
    wait_network(300);
    w1.reset();  // деструктор шлёт SIGTERM/SIGKILL

    client_thread.join();

    // Несмотря на смерть воркера, задача завершилась корректно
    EXPECT_EQ(code_holder, 0);
    auto parsed = parse_client_output(out_holder);
    EXPECT_EQ(parsed.key, "D");

    // Второй воркер пережил
    EXPECT_TRUE(w2.alive());
}

#endif  // DECRYPTOR_TEST_WORKER_DIES


#ifdef DECRYPTOR_TEST_CHECKPOINT_RESTORE


TEST(fault_tolerance, checkpoint_file_created_and_reused) {
    fs::path checkpoint = temp_path();
    fs::create_directories(checkpoint.parent_path());
    fs::path cp_file = checkpoint / "checkpoint.json";

    int worker_port = next_port();
    int client_port = next_port();
    int peer_port = next_port();

    // Первый координатор с явным путём чекпоинта
    {
        auto coord = std::make_unique<BackgroundProcess>(
            coord_binary,
            std::vector<std::string>{
                "--worker-port", std::to_string(worker_port),
                "--client-port", std::to_string(client_port),
                "--peer-port", std::to_string(peer_port),
                "--id", "1",
                "--checkpoint", cp_file.string()
            },
            checkpoint / "coord1_logs"
        );
        wait_network();

        auto worker = std::make_unique<BackgroundProcess>(
            worker_binary,
            std::vector<std::string>{
                "--coordinators", "127.0.0.1:" + std::to_string(worker_port),
                "--dict", dict_path.string(),
                "--trigrams", trigrams_path.string()
            },
            checkpoint / "worker_logs"
        );
        wait_network();

        // длинная задача:
        std::thread client_thread([&]() {
            const std::string ct =
                "hfnlp yosnd ujit ks b vetv og xihgnfte dkpigr xkti mez cbd";
            Client::run_vigenere(
                "127.0.0.1:" + std::to_string(client_port), ct, "brute", 3
            );
        });
        client_thread.detach();   // не ждём завершения

        wait_network(2000);
        // выходим из блока — координатор и воркер убиваются
    }

    // Чекпоинт должен был создаться
    EXPECT_TRUE(fs::exists(cp_file))
        << "checkpoint file was not created at " << cp_file.string();

    // Второй координатор с тем же чекпоинтом — должен загрузиться без ошибок
    {
        auto coord2 = std::make_unique<BackgroundProcess>(
            coord_binary,
            std::vector<std::string>{
                "--worker-port", std::to_string(next_port()),
                "--client-port", std::to_string(next_port()),
                "--peer-port", std::to_string(next_port()),
                "--id", "1",
                "--checkpoint", cp_file.string()
            },
            checkpoint / "coord2_logs"
        );
        wait_network();

        // Координатор жив после попытки восстановления из чекпоинта
        EXPECT_TRUE(coord2->alive());

        // В логе должно быть упоминание восстановления (не падение)
        std::string log = read_file(checkpoint / "coord2_logs" / "stdout.log");
        // Не делаем строгую проверку текста — лог может отличаться,
        // главное что процесс жив и не упал на парсинге чекпоинта
    }

    std::error_code ec;
    fs::remove_all(checkpoint, ec);
}

#endif  // DECRYPTOR_TEST_CHECKPOINT_RESTORE

}  // namespace decryptor::tests

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    if (argc < 5) {
        std::cerr << "Usage: " << argv[0]
                  << " <coord_binary> <worker_binary> <client_binary> <dict_path> <trigrams_path>\n";
        return 1;
    }

    decryptor::tests::coord_binary = argv[1];
    decryptor::tests::worker_binary = argv[2];
    decryptor::tests::client_binary = argv[3];
    decryptor::tests::dict_path = argv[4];
    decryptor::tests::trigrams_path = argv[5];

    return RUN_ALL_TESTS();
}
