#pragma once

#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

using json = nlohmann::json;

class Base64 {
public:
    // Кодирование vector<byte> в Base64 строку
    static std::string encode(const std::vector<uint8_t> &data) {
        if (data.empty()) {
            return "";
        }

        BIO *bio;
        BIO *b64;
        BUF_MEM *bufferPtr;

        b64 = BIO_new(BIO_f_base64());
        bio = BIO_new(BIO_s_mem());
        bio = BIO_push(b64, bio);

        // Отключаем перевод строки (чтобы всё было в одну строку)
        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

        // Записываем данные
        BIO_write(bio, data.data(), static_cast<int>(data.size()));
        BIO_flush(bio);

        // Получаем указатель на данные в памяти
        BIO_get_mem_ptr(bio, &bufferPtr);

        // Копируем в std::string
        std::string result(bufferPtr->data, bufferPtr->length);

        // Очищаем
        BIO_free_all(bio);

        return result;
    }

    // Декодирование Base64 строки в vector<byte>
    static std::vector<uint8_t> decode(const std::string &b64) {
        if (b64.empty()) {
            return {};
        }

        BIO *bio;
        BIO *b64bio;

        b64bio = BIO_new(BIO_f_base64());
        bio = BIO_new_mem_buf(b64.data(), static_cast<int>(b64.length()));
        bio = BIO_push(b64bio, bio);

        // Отключаем перевод строки
        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

        // Определяем максимальный размер декодированных данных
        // Base64 добавляет ~33% overhead
        std::vector<uint8_t> result(b64.length());  // выделяем с запасом

        // Декодируем
        int decoded_len =
            BIO_read(bio, result.data(), static_cast<int>(result.size()));

        if (decoded_len < 0) {
            BIO_free_all(bio);
            throw std::runtime_error("Failed to decode Base64");
        }

        result.resize(decoded_len);  // обрезаем до реального размера

        BIO_free_all(bio);

        return result;
    }
};

namespace nlohmann {
// Специализация для vector<uint8_t>
template <>
struct adl_serializer<std::vector<uint8_t>> {
    static void to_json(json &j, const std::vector<uint8_t> &data) {
        j = Base64::encode(data);
    }

    static void from_json(const json &j, std::vector<uint8_t> &data) {
        data = Base64::decode(j.get<std::string>());
    }
};
}  // namespace nlohmann
