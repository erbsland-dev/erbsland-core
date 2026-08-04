// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyResponseReader.hpp"
#include "CryptologyTestHelper.hpp"

#include <erbsland/cryptology/keys/PublicKey.hpp>
#include <erbsland/cryptology/x509/X509AlgorithmIdentifier.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>

#include <array>
#include <format>

using namespace el::cryptology;
using namespace el::text::literals;

TESTED_TARGETS(RsaSignature PublicKey)
class RsaSignatureFullTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testAllPinnedC2spWycheproofCases() {
        const auto files = std::array<el::String, 7U>{
            "data/cryptology/signature/PSS_SHA256_SALT0.rsp"_el,
            "data/cryptology/signature/PSS_SHA256.rsp"_el,
            "data/cryptology/signature/PSS_SHA256_PSS_KEY_SALT0.rsp"_el,
            "data/cryptology/signature/PSS_SHA256_PSS_KEY_SALT32.rsp"_el,
            "data/cryptology/signature/PSS_SHA384.rsp"_el,
            "data/cryptology/signature/PKCS1_SHA256.rsp"_el,
            "data/cryptology/signature/PKCS1_SHA384.rsp"_el,
        };
        auto recordCount = std::size_t{};
        for (const auto &file : files) {
            const auto records = CryptologyResponseReader{file}.read();
            recordCount += records.size();
            for (const auto &record : records) {
                runWithContext(
                    SOURCE_LOCATION(),
                    [&]() -> void { validateRecord(record); },
                    [&]() -> std::string {
                        return std::format(
                            "{} tcId {} flags {}",
                            el::StringConverter{record.diagnostic()}.toStdString(),
                            el::StringConverter{record.value("Count"_el)}.toStdString(),
                            el::StringConverter{record.value("Flags"_el)}.toStdString());
                    });
            }
        }
        REQUIRE_EQUAL(recordCount, std::size_t{1080U});
    }

private:
    void validateRecord(const CryptologyResponseReader::Record &record) {
        record.requireAllowedSettings({"Padding"_el, "Hash"_el, "SaltLength"_el, "PublicKeyDer"_el, "AlgorithmDer"_el});
        record.requireAllowedValues({"Count"_el, "Result"_el, "Flags"_el, "Msg"_el, "Sig"_el});
        record.requireValues({"Count"_el, "Result"_el, "Flags"_el, "Msg"_el, "Sig"_el});
        const auto publicKey = PublicKey::fromDerOrThrow(bytesFromHex(record.setting("PublicKeyDer"_el)));
        const auto algorithm = X509AlgorithmIdentifier::fromDerOrThrow(bytesFromHex(record.setting("AlgorithmDer"_el)));
        const auto message = bytesFromHex(record.value("Msg"_el));
        const auto signature = bytesFromHex(record.value("Sig"_el));
        const auto expected = record.value("Result"_el) == "valid"_el;
        auto actual = false;
        try {
            actual = publicKey.verifySignature(algorithm, message.span(), signature.span());
        } catch (const el::err::ParseError &) {
            REQUIRE_FALSE(expected && !record.value("Flags"_el).contains("SmallPublicKey"_el));
            return;
        }
        REQUIRE_EQUAL(actual, expected);
    }
};
