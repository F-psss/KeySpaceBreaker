#ifndef UNIT_HPP
#define UNIT_HPP
#include <iostream>
#include <string>
#include "enums.hpp"

namespace server {

enum class UnitStatus { Unassigned, Leased, Done };

struct Result {
    int key_;
    double score_;
    std::string text_{};
};

class Unit {
public:
    Unit(
        int start,
        int end,
        decrypt::CipherType cipher,
        decrypt::VigenereMode mode,
        int key_length,
        double noise = 0.0
    )
        : m_start(start),
          m_end(end),
          m_cipher(cipher),
          m_mode(mode),
          m_key_length(key_length),
          m_noise(noise) {
    }

    [[nodiscard]] int get_start() const {
        return m_start;
    }

    [[nodiscard]] int get_end() const {
        return m_end;
    }

    [[nodiscard]] UnitStatus get_status() const {
        return m_status;
    }

    [[nodiscard]] bool is_end() const {
        return (m_start == -1) && (m_end == -1);
    }

    [[nodiscard]] double get_noise() const {
        return m_noise;
    }

    [[nodiscard]] decrypt::VigenereMode get_mode() const {
        return m_mode;
    }

    [[nodiscard]] decrypt::CipherType get_cipher() const {
        return m_cipher;
    }

    [[nodiscard]] int get_key_length() const {
        return m_key_length;
    }

    void mark_as_leased() {
        m_status = UnitStatus::Leased;
    }

    void mark_as_unassigned() {
        m_status = UnitStatus::Unassigned;
    }

    void mark_as_done() {
        m_status = UnitStatus::Done;
    }

private:
    int m_start;
    int m_end;
    UnitStatus m_status = UnitStatus::Unassigned;
    decrypt::CipherType m_cipher;
    decrypt::VigenereMode m_mode;
    int m_key_length;
    double m_noise;
};

}  // namespace server

#endif  // UNIT_HPP
