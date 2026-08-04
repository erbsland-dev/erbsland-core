// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyResponseReader.hpp"
#include "CryptologyTestHelper.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/cryptology/symmetric/SymmetricDecryptor.hpp>
#include <erbsland/cryptology/symmetric/SymmetricEncryptor.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/text/StringLiteral.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <string>

using namespace el::cryptology;
using namespace el::text::literals;
using el::mem::ByteBlock;
using el::mem::ByteBlockEditor;

TESTED_TARGETS(AesCbcState AesCbcEncryptorData AesCbcDecryptorData)
class AesCbcFullTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
private:
    struct TestFile final {
        el::StringLiteral name;
        std::size_t records;
        bool monteCarlo;
    };

    inline static constexpr auto files = std::array{
        TestFile{"CBCGFSbox256.rsp"_el, 10U, false},
        TestFile{"CBCKeySbox256.rsp"_el, 32U, false},
        TestFile{"CBCVarKey256.rsp"_el, 512U, false},
        TestFile{"CBCVarTxt256.rsp"_el, 256U, false},
        TestFile{"CBCMMT256.rsp"_el, 20U, false},
        TestFile{"CBCMCT256.rsp"_el, 200U, true},
    };

    [[nodiscard]] static auto keyFrom(const CryptologyResponseReader::Record &record) -> SymmetricKey {
        return SymmetricKey{bytesFromHex(record.value("KEY"_el))};
    }

    [[nodiscard]] static auto ivFrom(const CryptologyResponseReader::Record &record) -> SymmetricIv {
        return SymmetricIv{bytesFromHex(record.value("IV"_el))};
    }

    void verifyKnownAnswer(const CryptologyResponseReader::Record &record, const bool chunked) {
        record.requireAllowedSettings({});
        record.requireAllowedValues({"COUNT"_el, "KEY"_el, "IV"_el, "PLAINTEXT"_el, "CIPHERTEXT"_el});
        record.requireValues({"COUNT"_el, "KEY"_el, "IV"_el, "PLAINTEXT"_el, "CIPHERTEXT"_el});
        const auto plaintext = bytesFromHex(record.value("PLAINTEXT"_el));
        const auto ciphertext = bytesFromHex(record.value("CIPHERTEXT"_el));
        REQUIRE_EQUAL(plaintext.length(), ciphertext.length());
        REQUIRE_EQUAL(plaintext.length().toSizeT() % 16U, 0U);

        if (record.direction == CryptologyResponseReader::Direction::Encrypt) {
            auto encryptor =
                SymmetricEncryptor{SymmetricEncryptionType::Aes256CbcRandomFill, keyFrom(record), ivFrom(record)};
            auto actual = ByteBlockEditor{};
            if (!chunked) {
                actual.append(encryptor.encrypt(plaintext));
            } else {
                auto offset = std::size_t{};
                while (offset < plaintext.length().toSizeT()) {
                    const auto count = std::min(((offset * 7U) % 29U) + 1U, plaintext.length().toSizeT() - offset);
                    actual.append(encryptor.encrypt(plaintext.span().subspan(offset, count)));
                    offset += count;
                }
            }
            actual.append(encryptor.finalize());
            REQUIRE_EQUAL(actual, ciphertext);
        } else {
            REQUIRE(record.direction == CryptologyResponseReader::Direction::Decrypt);
            auto decryptor =
                SymmetricDecryptor{SymmetricEncryptionType::Aes256CbcRandomFill, keyFrom(record), ivFrom(record)};
            auto actual = ByteBlockEditor{};
            if (!chunked) {
                actual.append(decryptor.decrypt(ciphertext));
            } else {
                auto offset = std::size_t{};
                while (offset < ciphertext.length().toSizeT()) {
                    const auto count = std::min(((offset * 11U) % 31U) + 1U, ciphertext.length().toSizeT() - offset);
                    actual.append(decryptor.decrypt(ciphertext.span().subspan(offset, count)));
                    offset += count;
                }
            }
            actual.append(decryptor.finalize());
            REQUIRE_EQUAL(actual, plaintext);
        }
    }

    void verifyMonteCarlo(const CryptologyResponseReader::Record &record) {
        record.requireAllowedSettings({});
        record.requireAllowedValues({"COUNT"_el, "KEY"_el, "IV"_el, "PLAINTEXT"_el, "CIPHERTEXT"_el});
        record.requireValues({"COUNT"_el, "KEY"_el, "IV"_el, "PLAINTEXT"_el, "CIPHERTEXT"_el});
        const auto initialIv = bytesFromHex(record.value("IV"_el));
        auto previous = ByteBlock{};

        if (record.direction == CryptologyResponseReader::Direction::Encrypt) {
            auto input = bytesFromHex(record.value("PLAINTEXT"_el));
            auto encryptor =
                SymmetricEncryptor{SymmetricEncryptionType::Aes256CbcRandomFill, keyFrom(record), ivFrom(record)};
            for (auto iteration = std::size_t{}; iteration < 1000U; ++iteration) {
                const auto output = encryptor.encrypt(input);
                REQUIRE_EQUAL(output.length(), el::unit::ByteLength{16U});
                input = iteration == 0U ? initialIv : previous;
                previous = output;
            }
            REQUIRE(encryptor.finalize().isEmpty());
            REQUIRE_EQUAL(previous, bytesFromHex(record.value("CIPHERTEXT"_el)));
        } else {
            REQUIRE(record.direction == CryptologyResponseReader::Direction::Decrypt);
            auto input = bytesFromHex(record.value("CIPHERTEXT"_el));
            auto decryptor =
                SymmetricDecryptor{SymmetricEncryptionType::Aes256CbcRandomFill, keyFrom(record), ivFrom(record)};
            for (auto iteration = std::size_t{}; iteration < 1000U; ++iteration) {
                const auto output = decryptor.decrypt(input);
                REQUIRE_EQUAL(output.length(), el::unit::ByteLength{16U});
                input = iteration == 0U ? initialIv : previous;
                previous = output;
            }
            REQUIRE(decryptor.finalize().isEmpty());
            REQUIRE_EQUAL(previous, bytesFromHex(record.value("PLAINTEXT"_el)));
        }
    }

public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testOfficialCbcVectors() {
        const auto applicationScope = ApplicationTestScope<>{};
        for (const auto &file : files) {
            const auto records = CryptologyResponseReader{el::Path{"data/cryptology/aes"_el} / file.name}.read();
            REQUIRE_EQUAL(records.size(), file.records);
            auto directionCounts = std::array<std::size_t, 2>{};
            for (const auto &record : records) {
                REQUIRE(
                    record.direction == CryptologyResponseReader::Direction::Encrypt ||
                    record.direction == CryptologyResponseReader::Direction::Decrypt);
                const auto directionIndex = record.direction == CryptologyResponseReader::Direction::Encrypt ? 0U : 1U;
                REQUIRE_EQUAL(record.unsignedValue("COUNT"_el), directionCounts[directionIndex]);
                ++directionCounts[directionIndex];
                if (file.monteCarlo) {
                    runWithContext(
                        SOURCE_LOCATION(),
                        [&]() -> void { verifyMonteCarlo(record); },
                        [&]() -> std::string { return el::StringConverter{record.diagnostic()}.toStdString(); });
                } else {
                    for (const auto chunked : {false, true}) {
                        runWithContext(
                            SOURCE_LOCATION(),
                            [&]() -> void { verifyKnownAnswer(record, chunked); },
                            [&]() -> std::string {
                                return std::format(
                                    "{} streaming {}", el::StringConverter{record.diagnostic()}.toStdString(), chunked);
                            });
                    }
                }
            }
            REQUIRE_EQUAL(directionCounts, (std::array<std::size_t, 2>{file.records / 2U, file.records / 2U}));
        }
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testIsoMethod2EveryTailLength() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto testKey = SymmetricKey{bytesFromHex(
            "603deb1015ca71be2b73aef0857d7781"
            "1f352c073b6108d72d9810a30914dff4"_el)};
        const auto testIv = SymmetricIv{bytesFromHex("000102030405060708090a0b0c0d0e0f"_el)};
        for (auto tailLength = std::size_t{}; tailLength < 16U; ++tailLength) {
            auto plaintext = ByteBlockEditor{el::unit::ByteLength::fromSizeT(32U + tailLength)};
            for (auto index = el::unit::ByteIndex{}; index < plaintext.endIndex(); ++index) {
                plaintext.set(index, el::mem::Byte{static_cast<uint8_t>(index.toSizeT() * 37U + tailLength)});
            }
            auto encryptor = SymmetricEncryptor{SymmetricEncryptionType::Aes256CbcIso9797Method2, testKey, testIv};
            auto ciphertext = ByteBlockEditor{};
            for (const auto byte : plaintext.span()) {
                ciphertext.append(encryptor.encrypt(el::mem::ConstByteSpan{&byte, 1U}));
            }
            ciphertext.append(encryptor.finalize());
            REQUIRE_EQUAL(ciphertext.length(), el::unit::ByteLength::fromSizeT(48U));

            auto decryptor = SymmetricDecryptor{SymmetricEncryptionType::Aes256CbcIso9797Method2, testKey, testIv};
            auto recovered = ByteBlockEditor{};
            auto offset = std::size_t{};
            while (offset < ciphertext.length().toSizeT()) {
                const auto count = std::min(std::size_t{5U}, ciphertext.length().toSizeT() - offset);
                recovered.append(decryptor.decrypt(ciphertext.span().subspan(offset, count)));
                offset += count;
            }
            recovered.append(decryptor.finalize());
            REQUIRE_EQUAL(recovered, plaintext);
        }
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testIsoPrefixMatchesOfficialCbc() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto records = CryptologyResponseReader{"data/cryptology/aes/CBCMMT256.rsp"_el}.read();
        for (const auto &record : records) {
            if (record.direction != CryptologyResponseReader::Direction::Encrypt) {
                continue;
            }
            const auto plaintext = bytesFromHex(record.value("PLAINTEXT"_el));
            const auto expectedPrefix = bytesFromHex(record.value("CIPHERTEXT"_el));
            auto encryptor =
                SymmetricEncryptor{SymmetricEncryptionType::Aes256CbcIso9797Method2, keyFrom(record), ivFrom(record)};
            auto ciphertext = ByteBlockEditor{encryptor.encrypt(plaintext)};
            ciphertext.append(encryptor.finalize());
            REQUIRE(ciphertext.startsWith(expectedPrefix.span()));
        }
    }
};
