// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "AesCbcTestApplication.hpp"
#include "CryptologyTestHelper.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/cryptology/CryptologyError.hpp>
#include <erbsland/cryptology/symmetric/SymmetricDecryptor.hpp>
#include <erbsland/cryptology/symmetric/SymmetricEncryptor.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>

using namespace el::cryptology;
using el::mem::ByteBlockEditor;

TESTED_TARGETS(AesCbcState AesCbcEncryptorData AesCbcDecryptorData SymmetricDataFactory)
class AesCbcTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
private:
    [[nodiscard]] static auto key() -> SymmetricKey {
        return SymmetricKey{bytesFromHex(
            "603deb1015ca71be2b73aef0857d7781"
            "1f352c073b6108d72d9810a30914dff4")};
    }

    [[nodiscard]] static auto iv() -> SymmetricIv {
        return SymmetricIv{bytesFromHex("000102030405060708090a0b0c0d0e0f")};
    }

public:
    void testSp80038aRandomFillAlignedVector() {
        const auto applicationScope = ApplicationTestScope<>{};
        // NIST SP 800-38A, Appendix F.2.5: AES-256-CBC with four aligned blocks.
        const auto plaintext = bytesFromHex(
            "6bc1bee22e409f96e93d7e117393172a"
            "ae2d8a571e03ac9c9eb76fac45af8e51"
            "30c81c46a35ce411e5fbc1191a0a52ef"
            "f69f2445df4f9b17ad2b417be66c3710");
        const auto expected = bytesFromHex(
            "f58c4c04d6e5f1ba779eabfb5f7bfbd6"
            "9cfc4e967edb808d679f777bc6702c7d"
            "39f23369a9d9bacfa530e26304231461"
            "b2eb05e2c39be9fcda6c19078c6a9d1b");
        auto encryptor = SymmetricEncryptor{SymmetricEncryptionType::Aes256CbcRandomFill, key(), iv()};
        auto ciphertext = ByteBlockEditor{};
        ciphertext.append(encryptor.encrypt(plaintext.span(el::unit::ByteIndex{}, el::unit::ByteLength{13U})));
        ciphertext.append(encryptor.encrypt(plaintext.span(el::unit::ByteIndex{13U}, el::unit::ByteLength{51U})));
        ciphertext.append(encryptor.finalize());
        REQUIRE_EQUAL(ciphertext, expected);

        auto decryptor = SymmetricDecryptor{SymmetricEncryptionType::Aes256CbcRandomFill, key(), iv()};
        auto recovered = ByteBlockEditor{};
        recovered.append(decryptor.decrypt(expected));
        recovered.append(decryptor.finalize());
        REQUIRE_EQUAL(recovered, plaintext);
    }

    void testIsoMethod2RoundTripAndEmptyInput() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto plaintext = bytesFromHex("00112233445566778899aabbccddeeff1020304050");
        auto encryptor = SymmetricEncryptor{SymmetricEncryptionType::Aes256CbcIso9797Method2, key(), iv()};
        auto ciphertext = ByteBlockEditor{};
        ciphertext.append(encryptor.encrypt(plaintext));
        ciphertext.append(encryptor.finalize());
        REQUIRE_EQUAL(ciphertext.length(), el::unit::ByteLength{32U});

        auto decryptor = SymmetricDecryptor{SymmetricEncryptionType::Aes256CbcIso9797Method2, key(), iv()};
        auto recovered = ByteBlockEditor{};
        for (auto index = std::size_t{}; index < ciphertext.length().toSizeT(); ++index) {
            recovered.append(
                decryptor.decrypt(ciphertext.span(el::unit::ByteIndex{index}, el::unit::ByteLength::one())));
        }
        recovered.append(decryptor.finalize());
        REQUIRE_EQUAL(recovered, plaintext);

        auto emptyEncryptor = SymmetricEncryptor{SymmetricEncryptionType::Aes256CbcIso9797Method2, key(), iv()};
        const auto emptyCiphertext = emptyEncryptor.finalize();
        REQUIRE_EQUAL(emptyCiphertext.length(), el::unit::ByteLength{16U});
        auto emptyDecryptor = SymmetricDecryptor{SymmetricEncryptionType::Aes256CbcIso9797Method2, key(), iv()};
        REQUIRE(emptyDecryptor.decrypt(emptyCiphertext).isEmpty());
        REQUIRE(emptyDecryptor.finalize().isEmpty());
    }

    void testCbcFinalizationErrors() {
        const auto applicationScope = ApplicationTestScope<>{};
        auto emptyRandom = SymmetricDecryptor{SymmetricEncryptionType::Aes256CbcRandomFill, key(), iv()};
        REQUIRE(emptyRandom.finalize().isEmpty());

        auto truncated = SymmetricDecryptor{SymmetricEncryptionType::Aes256CbcRandomFill, key(), iv()};
        static_cast<void>(truncated.decrypt(bytesFromHex("001122")));
        REQUIRE_THROWS_AS(CryptologyError, truncated.finalize());

        auto emptyIso = SymmetricDecryptor{SymmetricEncryptionType::Aes256CbcIso9797Method2, key(), iv()};
        REQUIRE_THROWS_AS(CryptologyError, emptyIso.finalize());

        auto malformedIso = SymmetricDecryptor{SymmetricEncryptionType::Aes256CbcIso9797Method2, key(), iv()};
        static_cast<void>(malformedIso.decrypt(bytesFromHex("00000000000000000000000000000000")));
        REQUIRE_THROWS_AS(CryptologyError, malformedIso.finalize());

        // Validate every position of an ISO method 2 suffix: the marker and each following zero byte.
        for (auto paddingIndex = std::size_t{7U}; paddingIndex < 16U; ++paddingIndex) {
            auto forgedPlaintext = ByteBlockEditor{bytesFromHex("00112233445566800000000000000000")};
            forgedPlaintext.set(
                el::unit::ByteIndex{paddingIndex}, paddingIndex == 7U ? el::mem::Byte{} : el::mem::Byte{0x01U});
            auto rawEncryptor = SymmetricEncryptor{SymmetricEncryptionType::Aes256CbcRandomFill, key(), iv()};
            auto forgedCiphertext = ByteBlockEditor{rawEncryptor.encrypt(forgedPlaintext)};
            forgedCiphertext.append(rawEncryptor.finalize());

            auto decryptor = SymmetricDecryptor{SymmetricEncryptionType::Aes256CbcIso9797Method2, key(), iv()};
            static_cast<void>(decryptor.decrypt(forgedCiphertext));
            REQUIRE_THROWS_AS(CryptologyError, decryptor.finalize());
        }
    }

    void testRandomFillKeepsFillBytes() {
        const auto applicationScope = ApplicationTestScope<el::test::AesCbcTestApplication>{};
        const auto plaintext = bytesFromHex("00112233445566");
        // Fast File Encryption compatibility fixture: a seven-byte plaintext followed by deterministic fill
        // 00..08, encrypted as raw AES-256-CBC without a padding transform.
        const auto expectedCiphertext = bytesFromHex("2a824ab570bdb5393b28f2572bf8f1a0");
        auto encryptor = SymmetricEncryptor{SymmetricEncryptionType::Aes256CbcRandomFill, key(), iv()};
        auto ciphertext = ByteBlockEditor{};
        ciphertext.append(encryptor.encrypt(plaintext));
        ciphertext.append(encryptor.finalize());
        REQUIRE_EQUAL(ciphertext.length(), el::unit::ByteLength{16U});
        REQUIRE_EQUAL(ciphertext, expectedCiphertext);

        auto decryptor = SymmetricDecryptor{SymmetricEncryptionType::Aes256CbcRandomFill, key(), iv()};
        auto recovered = ByteBlockEditor{decryptor.decrypt(ciphertext)};
        recovered.append(decryptor.finalize());
        REQUIRE_EQUAL(recovered.length(), el::unit::ByteLength{16U});
        REQUIRE(recovered.startsWith(plaintext.span()));
    }
};
