// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyTestHelper.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/cryptology/CryptologyError.hpp>
#include <erbsland/cryptology/symmetric/SymmetricDecryptor.hpp>
#include <erbsland/cryptology/symmetric/SymmetricEncryptor.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>

using namespace el::cryptology;
using el::mem::ByteBlock;
using el::mem::ByteBlockEditor;

TESTED_TARGETS(AesGcmState AesGcmEncryptorData AesGcmDecryptorData GHash SymmetricDataFactory)
class AesGcmTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
private:
    [[nodiscard]] static auto key(const std::string_view hex) -> SymmetricKey {
        return SymmetricKey{bytesFromHex(hex)};
    }

    [[nodiscard]] static auto nonce(const std::string_view hex) -> SymmetricNonce {
        return SymmetricNonce{bytesFromHex(hex)};
    }

public:
    void testEmptyNistVector() {
        const auto applicationScope = ApplicationTestScope<>{};
        // NIST SP 800-38D, Appendix B / GCMVS: AES-128, empty AAD and empty plaintext.
        const auto testKey = key("00000000000000000000000000000000");
        const auto testNonce = nonce("000000000000000000000000");
        auto encryptor = SymmetricEncryptor{SymmetricEncryptionType::Aes128Gcm, testKey, testNonce};
        REQUIRE(encryptor.finalize().isEmpty());
        REQUIRE_EQUAL(encryptor.tag().data(), bytesFromHex("58e2fccefa7e3061367f1d57a4e7455a"));

        auto decryptor = SymmetricDecryptor{SymmetricEncryptionType::Aes128Gcm, testKey, testNonce};
        REQUIRE(decryptor.finalize(encryptor.tag()).isEmpty());
    }

    void testSingleBlockNistVectorAndInvalidTag() {
        const auto applicationScope = ApplicationTestScope<>{};
        // NIST SP 800-38D, Test Case 2: one all-zero AES-128-GCM plaintext block.
        const auto testKey = key("00000000000000000000000000000000");
        const auto testNonce = nonce("000000000000000000000000");
        const auto plaintext = bytesFromHex("00000000000000000000000000000000");
        const auto expectedCiphertext = bytesFromHex("0388dace60b6a392f328c2b971b2fe78");
        const auto expectedTag = bytesFromHex("ab6e47d42cec13bdf53a67b21257bddf");

        auto encryptor = SymmetricEncryptor{SymmetricEncryptionType::Aes128Gcm, testKey, testNonce};
        REQUIRE_EQUAL(encryptor.encrypt(plaintext), expectedCiphertext);
        REQUIRE(encryptor.finalize().isEmpty());
        REQUIRE_EQUAL(encryptor.tag().data(), expectedTag);

        auto decryptor = SymmetricDecryptor{SymmetricEncryptionType::Aes128Gcm, testKey, testNonce};
        REQUIRE_EQUAL(decryptor.decrypt(expectedCiphertext), plaintext);
        REQUIRE(decryptor.finalize(SymmetricTag{expectedTag}).isEmpty());

        auto invalidTag = ByteBlockEditor{expectedTag};
        invalidTag.xorAt(el::unit::ByteIndex{}, el::mem::Byte{1U});
        auto rejectingDecryptor = SymmetricDecryptor{SymmetricEncryptionType::Aes128Gcm, testKey, testNonce};
        static_cast<void>(rejectingDecryptor.decrypt(expectedCiphertext));
        REQUIRE_THROWS_AS(CryptologyError, rejectingDecryptor.finalize(SymmetricTag{invalidTag}));
    }

    void testAadOnlyNistGcmvsVector() {
        const auto applicationScope = ApplicationTestScope<>{};
        // NIST CAVP GCMVS gcmEncryptExtIV128.rsp: PTlen=0, AADlen=128, Taglen=128, Count=0.
        const auto testKey = key("77be63708971c4e240d1cb79e8d77feb");
        const auto testNonce = nonce("e0e00f19fed7ba0136a797f3");
        const auto aad = bytesFromHex("7a43ec1d9c0a5a78a0b16533a6213cab");
        const auto expectedTag = bytesFromHex("209fcc8d3675ed938e9c7166709dd946");

        auto encryptor = SymmetricEncryptor{SymmetricEncryptionType::Aes128Gcm, testKey, testNonce};
        encryptor.addAuthenticatedData(aad);
        REQUIRE(encryptor.finalize().isEmpty());
        REQUIRE_EQUAL(encryptor.tag().data(), expectedTag);

        auto decryptor = SymmetricDecryptor{SymmetricEncryptionType::Aes128Gcm, testKey, testNonce};
        decryptor.addAuthenticatedData(aad);
        REQUIRE(decryptor.finalize(SymmetricTag{expectedTag}).isEmpty());
    }

    void testPartialAadAndIrregularStreaming() {
        const auto applicationScope = ApplicationTestScope<>{};
        // NIST SP 800-38D, Test Case 4: partial final payload block and 20 bytes of AAD.
        const auto testKey = key("feffe9928665731c6d6a8f9467308308");
        const auto testNonce = nonce("cafebabefacedbaddecaf888");
        const auto aad = bytesFromHex("feedfacedeadbeeffeedfacedeadbeefabaddad2");
        const auto plaintext = bytesFromHex(
            "d9313225f88406e5a55909c5aff5269a86a7a9531534f7da2e4c303d8a318a72"
            "1c3c0c95956809532fcf0e2449a6b525b16aedf5aa0de657ba637b39");
        const auto expectedCiphertext = bytesFromHex(
            "42831ec2217774244b7221b784d0d49c"
            "e3aa212f2c02a4e035c17e2329aca12e"
            "21d514b25466931c7d8f6a5aac84aa05"
            "1ba30b396a0aac973d58e091");
        const auto expectedTag = bytesFromHex("5bc94fbc3221a5db94fae95ae7121a47");

        auto encryptor = SymmetricEncryptor{SymmetricEncryptionType::Aes128Gcm, testKey, testNonce};
        encryptor.addAuthenticatedData(aad.span(el::unit::ByteIndex{}, el::unit::ByteLength{3U}));
        encryptor.addAuthenticatedData(aad.span(el::unit::ByteIndex{3U}, el::unit::ByteLength{17U}));
        auto ciphertext = ByteBlockEditor{};
        for (auto index = std::size_t{}; index < plaintext.length().toSizeT(); ++index) {
            ciphertext.append(
                encryptor.encrypt(plaintext.span(el::unit::ByteIndex{index}, el::unit::ByteLength::one())));
        }
        ciphertext.append(encryptor.finalize());
        REQUIRE_EQUAL(ciphertext, expectedCiphertext);
        REQUIRE_EQUAL(encryptor.tag().data(), expectedTag);

        auto decryptor = SymmetricDecryptor{SymmetricEncryptionType::Aes128Gcm, testKey, testNonce};
        decryptor.addAuthenticatedData(aad);
        auto recovered = ByteBlockEditor{};
        recovered.append(decryptor.decrypt(expectedCiphertext.span(el::unit::ByteIndex{}, el::unit::ByteLength{7U})));
        recovered.append(
            decryptor.decrypt(expectedCiphertext.span(el::unit::ByteIndex{7U}, el::unit::ByteLength{53U})));
        recovered.append(decryptor.finalize(SymmetricTag{expectedTag}));
        REQUIRE_EQUAL(recovered, plaintext);
    }

    void testAes256Vector() {
        const auto applicationScope = ApplicationTestScope<>{};
        // NIST GCMVS AES-256 case with one all-zero plaintext block.
        const auto testKey = key("0000000000000000000000000000000000000000000000000000000000000000");
        const auto testNonce = nonce("000000000000000000000000");
        auto encryptor = SymmetricEncryptor{SymmetricEncryptionType::Aes256Gcm, testKey, testNonce};
        REQUIRE_EQUAL(
            encryptor.encrypt(bytesFromHex("00000000000000000000000000000000")),
            bytesFromHex("cea7403d4d606b6e074ec5d3baf39d18"));
        static_cast<void>(encryptor.finalize());
        REQUIRE_EQUAL(encryptor.tag().data(), bytesFromHex("d0d1c8a799996bf0265b98b5d48ab919"));
    }
};
