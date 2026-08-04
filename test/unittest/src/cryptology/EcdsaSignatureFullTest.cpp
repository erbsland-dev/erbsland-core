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

TESTED_TARGETS(EcdsaSignature NistPrimeCurve PublicKey)
class EcdsaSignatureFullTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testAllPinnedNistAndC2spWycheproofCases() {
        const auto files = std::array<el::String, 3U>{
            "data/cryptology/signature/ECDSA_NIST_SIGVER.rsp"_el,
            "data/cryptology/signature/ECDSA_P256_SHA256.rsp"_el,
            "data/cryptology/signature/ECDSA_P384_SHA384.rsp"_el,
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
                            "{} count {} flags {}",
                            el::StringConverter{record.diagnostic()}.toStdString(),
                            el::StringConverter{record.value("Count"_el)}.toStdString(),
                            el::StringConverter{record.value("Flags"_el)}.toStdString());
                    });
            }
        }
        REQUIRE_EQUAL(recordCount, std::size_t{1018U});
    }

private:
    void validateRecord(const CryptologyResponseReader::Record &record) {
        record.requireAllowedSettings({"Curve"_el, "Hash"_el, "PublicKeyDer"_el, "AlgorithmDer"_el});
        record.requireAllowedValues({"Count"_el, "Result"_el, "Flags"_el, "Msg"_el, "Sig"_el});
        record.requireValues({"Count"_el, "Result"_el, "Flags"_el, "Msg"_el, "Sig"_el});
        const auto expected = record.value("Result"_el) == "valid"_el;
        auto actual = false;
        try {
            const auto publicKey = PublicKey::fromDerOrThrow(bytesFromHex(record.setting("PublicKeyDer"_el)));
            const auto algorithm =
                X509AlgorithmIdentifier::fromDerOrThrow(bytesFromHex(record.setting("AlgorithmDer"_el)));
            const auto message = bytesFromHex(record.value("Msg"_el));
            const auto signature = bytesFromHex(record.value("Sig"_el));
            actual = publicKey.verifySignature(algorithm, message.span(), signature.span());
        } catch (const el::err::ParseError &) {
            REQUIRE_FALSE(expected);
            return;
        }
        REQUIRE_EQUAL(actual, expected);
    }
};
