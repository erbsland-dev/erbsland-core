// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyResponseReader.hpp"
#include "CryptologyTestHelper.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/cryptology/CryptologyError.hpp>
#include <erbsland/cryptology/symmetric/SymmetricDecryptor.hpp>
#include <erbsland/cryptology/symmetric/SymmetricEncryptor.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/text/StringLiteral.hpp>

#include <algorithm>
#include <array>
#include <format>
#include <map>
#include <string>

using namespace el::cryptology;
using namespace el::text::literals;
using el::mem::ByteBlock;
using el::mem::ByteBlockEditor;

TESTED_TARGETS(AesGcmState AesGcmEncryptorData AesGcmDecryptorData GHash)
class AesGcmFullTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
private:
    struct TestFile final {
        el::StringLiteral name;
        std::size_t keyBits;
        bool encrypting;
    };

    inline static constexpr auto files = std::array{
        TestFile{"gcmEncryptExtIV128.rsp"_el, 128U, true},
        TestFile{"gcmEncryptExtIV256.rsp"_el, 256U, true},
        TestFile{"gcmDecrypt128.rsp"_el, 128U, false},
        TestFile{"gcmDecrypt256.rsp"_el, 256U, false},
    };

    [[nodiscard]] static auto typeFor(const std::size_t keyBits) -> SymmetricEncryptionType {
        return keyBits == 128U ? SymmetricEncryptionType::Aes128Gcm : SymmetricEncryptionType::Aes256Gcm;
    }

    [[nodiscard]] static auto keyFrom(const CryptologyResponseReader::Record &record) -> SymmetricKey {
        return SymmetricKey{bytesFromHex(record.value("Key"_el))};
    }

    [[nodiscard]] static auto nonceFrom(const CryptologyResponseReader::Record &record) -> SymmetricNonce {
        return SymmetricNonce{bytesFromHex(record.value("IV"_el))};
    }

    static void addAad(SymmetricEncryptor &encryptor, const ByteBlock &aad, const bool chunked) {
        if (!chunked) {
            encryptor.addAuthenticatedData(aad);
            return;
        }
        for (auto offset = std::size_t{}; offset < aad.length().toSizeT();) {
            const auto count = std::min(((offset * 5U) % 17U) + 1U, aad.length().toSizeT() - offset);
            encryptor.addAuthenticatedData(aad.span().subspan(offset, count));
            offset += count;
        }
    }

    static void addAad(SymmetricDecryptor &decryptor, const ByteBlock &aad, const bool chunked) {
        if (!chunked) {
            decryptor.addAuthenticatedData(aad);
            return;
        }
        for (auto offset = std::size_t{}; offset < aad.length().toSizeT();) {
            const auto count = std::min(((offset * 7U) % 19U) + 1U, aad.length().toSizeT() - offset);
            decryptor.addAuthenticatedData(aad.span().subspan(offset, count));
            offset += count;
        }
    }

    [[nodiscard]] static auto encryptPayload(
        SymmetricEncryptor &encryptor, const ByteBlock &plaintext, const bool chunked) -> ByteBlockEditor {
        auto result = ByteBlockEditor{};
        if (!chunked) {
            result.append(encryptor.encrypt(plaintext));
        } else {
            for (auto offset = std::size_t{}; offset < plaintext.length().toSizeT();) {
                const auto count = std::min(((offset * 11U) % 23U) + 1U, plaintext.length().toSizeT() - offset);
                result.append(encryptor.encrypt(plaintext.span().subspan(offset, count)));
                offset += count;
            }
        }
        result.append(encryptor.finalize());
        return result;
    }

    [[nodiscard]] static auto decryptPayload(
        SymmetricDecryptor &decryptor, const ByteBlock &ciphertext, const bool chunked) -> ByteBlockEditor {
        auto result = ByteBlockEditor{};
        if (!chunked) {
            result.append(decryptor.decrypt(ciphertext));
        } else {
            for (auto offset = std::size_t{}; offset < ciphertext.length().toSizeT();) {
                const auto count = std::min(((offset * 13U) % 29U) + 1U, ciphertext.length().toSizeT() - offset);
                result.append(decryptor.decrypt(ciphertext.span().subspan(offset, count)));
                offset += count;
            }
        }
        return result;
    }

    void verifyDimensions(const CryptologyResponseReader::Record &record, const TestFile &file) {
        record.requireAllowedSettings({"Keylen"_el, "IVlen"_el, "PTlen"_el, "AADlen"_el, "Taglen"_el});
        REQUIRE_EQUAL(record.unsignedSetting("Keylen"_el), file.keyBits);
        REQUIRE_EQUAL(record.unsignedSetting("IVlen"_el), 96U);
        REQUIRE_EQUAL(record.unsignedSetting("Taglen"_el), 128U);
        REQUIRE_EQUAL(bytesFromHex(record.value("IV"_el)).length().toSizeT() * 8U, record.unsignedSetting("IVlen"_el));
        REQUIRE_EQUAL(
            bytesFromHex(record.value("AAD"_el)).length().toSizeT() * 8U, record.unsignedSetting("AADlen"_el));
        REQUIRE_EQUAL(bytesFromHex(record.value("CT"_el)).length().toSizeT() * 8U, record.unsignedSetting("PTlen"_el));
        REQUIRE_EQUAL(
            bytesFromHex(record.value("Tag"_el)).length().toSizeT() * 8U, record.unsignedSetting("Taglen"_el));
    }

    void verifyEncryption(const CryptologyResponseReader::Record &record, const TestFile &file, const bool chunked) {
        record.requireAllowedValues({"Count"_el, "Key"_el, "IV"_el, "PT"_el, "AAD"_el, "CT"_el, "Tag"_el});
        record.requireValues({"Count"_el, "Key"_el, "IV"_el, "PT"_el, "AAD"_el, "CT"_el, "Tag"_el});
        verifyDimensions(record, file);
        const auto plaintext = bytesFromHex(record.value("PT"_el));
        REQUIRE_EQUAL(plaintext.length().toSizeT() * 8U, record.unsignedSetting("PTlen"_el));
        auto encryptor = SymmetricEncryptor{typeFor(file.keyBits), keyFrom(record), nonceFrom(record)};
        addAad(encryptor, bytesFromHex(record.value("AAD"_el)), chunked);
        REQUIRE_EQUAL(encryptPayload(encryptor, plaintext, chunked), bytesFromHex(record.value("CT"_el)));
        REQUIRE_EQUAL(encryptor.tag().data(), bytesFromHex(record.value("Tag"_el)));
    }

    void verifyDecryption(const CryptologyResponseReader::Record &record, const TestFile &file, const bool chunked) {
        record.requireAllowedValues({"Count"_el, "Key"_el, "IV"_el, "CT"_el, "AAD"_el, "Tag"_el, "PT"_el});
        record.requireValues({"Count"_el, "Key"_el, "IV"_el, "CT"_el, "AAD"_el, "Tag"_el});
        verifyDimensions(record, file);
        const auto ciphertext = bytesFromHex(record.value("CT"_el));
        auto decryptor = SymmetricDecryptor{typeFor(file.keyBits), keyFrom(record), nonceFrom(record)};
        addAad(decryptor, bytesFromHex(record.value("AAD"_el)), chunked);
        auto plaintext = decryptPayload(decryptor, ciphertext, chunked);
        const auto tag = SymmetricTag{bytesFromHex(record.value("Tag"_el))};
        if (record.failed) {
            REQUIRE_FALSE(record.hasValue("PT"_el));
            REQUIRE_THROWS_AS(CryptologyError, decryptor.finalize(tag));
            return;
        }
        record.requireValues({"PT"_el});
        plaintext.append(decryptor.finalize(tag));
        REQUIRE_EQUAL(plaintext, bytesFromHex(record.value("PT"_el)));
    }

public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testOfficialGcmVectors() {
        const auto applicationScope = ApplicationTestScope<>{};
        for (const auto &file : files) {
            const auto records = CryptologyResponseReader{el::Path{"data/cryptology/gcm"_el} / file.name}.read();
            REQUIRE_EQUAL(records.size(), 375U);
            auto dimensions = std::map<std::pair<std::size_t, std::size_t>, std::size_t>{};
            for (const auto &record : records) {
                const auto dimension =
                    std::pair{record.unsignedSetting("PTlen"_el), record.unsignedSetting("AADlen"_el)};
                REQUIRE_EQUAL(record.unsignedValue("Count"_el), dimensions[dimension]);
                ++dimensions[dimension];
                for (const auto chunked : {false, true}) {
                    runWithContext(
                        SOURCE_LOCATION(),
                        [&]() -> void {
                            if (file.encrypting) {
                                verifyEncryption(record, file, chunked);
                            } else {
                                verifyDecryption(record, file, chunked);
                            }
                        },
                        [&]() -> std::string {
                            return std::format(
                                "{} streaming {}", el::StringConverter{record.diagnostic()}.toStdString(), chunked);
                        });
                }
            }
            REQUIRE_EQUAL(dimensions.size(), 25U);
            for (const auto &[dimension, count] : dimensions) {
                static_cast<void>(dimension);
                REQUIRE_EQUAL(count, 15U);
            }
        }
    }
};
