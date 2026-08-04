// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyResponseReader.hpp"
#include "CryptologyTestHelper.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/cryptology/CryptologyError.hpp>
#include <erbsland/cryptology/symmetric/SymmetricDecryptor.hpp>
#include <erbsland/cryptology/symmetric/SymmetricEncryptor.hpp>
#include <erbsland/err/ParameterError.hpp>

#include <filesystem>
#include <format>

using namespace el::cryptology;
using namespace el::text::literals;

TESTED_TARGETS(ChaCha20Poly1305State ChaCha20Poly1305EncryptorData ChaCha20Poly1305DecryptorData)
class ChaCha20Poly1305FullTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testAllPinnedC2spWycheproofCases() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto records = CryptologyResponseReader{"data/cryptology/chacha20_poly1305/wycheproof.rsp"_el}.read();
        REQUIRE_EQUAL(records.size(), 325U);
        auto validCount = std::size_t{};
        auto authenticationFailureCount = std::size_t{};
        auto invalidNonceCount = std::size_t{};
        for (const auto &record : records) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() -> void { validateRecord(record, validCount, authenticationFailureCount, invalidNonceCount); },
                [&]() -> std::string {
                    return std::format(
                        "{} tcId {} flags {}",
                        el::StringConverter{record.diagnostic()}.toStdString(),
                        el::StringConverter{record.value("Count"_el)}.toStdString(),
                        el::StringConverter{record.value("Flags"_el)}.toStdString());
                });
        }
        REQUIRE_EQUAL(validCount, 256U);
        REQUIRE_EQUAL(authenticationFailureCount, 60U);
        REQUIRE_EQUAL(invalidNonceCount, 9U);
    }

private:
    void validateRecord(
        const CryptologyResponseReader::Record &record,
        std::size_t &validCount,
        std::size_t &authenticationFailureCount,
        std::size_t &invalidNonceCount) {
        record.requireAllowedSettings({"IVlen"_el, "Keylen"_el, "Taglen"_el});
        record.requireAllowedValues(
            {"Count"_el, "Result"_el, "Flags"_el, "Key"_el, "Nonce"_el, "AAD"_el, "Msg"_el, "CT"_el, "Tag"_el});
        record.requireValues(
            {"Count"_el, "Result"_el, "Flags"_el, "Key"_el, "Nonce"_el, "AAD"_el, "Msg"_el, "CT"_el, "Tag"_el});
        REQUIRE_EQUAL(record.unsignedSetting("Keylen"_el), 256U);
        REQUIRE_EQUAL(record.unsignedSetting("Taglen"_el), 128U);
        const auto key = SymmetricKey{bytesFromHex(record.value("Key"_el))};
        const auto nonce = SymmetricNonce{bytesFromHex(record.value("Nonce"_el))};
        const auto aad = bytesFromHex(record.value("AAD"_el));
        const auto message = bytesFromHex(record.value("Msg"_el));
        const auto ciphertext = bytesFromHex(record.value("CT"_el));
        const auto tag = SymmetricTag{bytesFromHex(record.value("Tag"_el))};

        if (record.unsignedSetting("IVlen"_el) != 96U) {
            ++invalidNonceCount;
            REQUIRE_EQUAL(record.value("Result"_el), "invalid"_el);
            REQUIRE_THROWS_AS(
                el::err::ParameterError, SymmetricEncryptor(SymmetricEncryptionType::ChaCha20Poly1305, key, nonce));
            REQUIRE_THROWS_AS(
                el::err::ParameterError, SymmetricDecryptor(SymmetricEncryptionType::ChaCha20Poly1305, key, nonce));
            return;
        }

        if (record.value("Result"_el) == "valid"_el) {
            ++validCount;
            auto encryptor = SymmetricEncryptor{SymmetricEncryptionType::ChaCha20Poly1305, key, nonce};
            encryptor.addAuthenticatedData(aad);
            REQUIRE_EQUAL(encryptor.encrypt(message), ciphertext);
            REQUIRE(encryptor.finalize().isEmpty());
            REQUIRE_EQUAL(encryptor.tag().data(), tag.data());

            auto decryptor = SymmetricDecryptor{SymmetricEncryptionType::ChaCha20Poly1305, key, nonce};
            decryptor.addAuthenticatedData(aad);
            REQUIRE_EQUAL(decryptor.decrypt(ciphertext), message);
            REQUIRE(decryptor.finalize(tag).isEmpty());
            return;
        }

        ++authenticationFailureCount;
        auto decryptor = SymmetricDecryptor{SymmetricEncryptionType::ChaCha20Poly1305, key, nonce};
        decryptor.addAuthenticatedData(aad);
        static_cast<void>(decryptor.decrypt(ciphertext));
        REQUIRE_THROWS_AS(CryptologyError, decryptor.finalize(tag));
    }
};
