// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyResponseKeys.hpp"
#include "CryptologyResponseReader.hpp"
#include "CryptologyTestHelper.hpp"

#include "../ParallelTestWork.hpp"

#include <erbsland/Literals.hpp>
#include <erbsland/ParseError.hpp>
#include <erbsland/PublicKey.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/X509AlgorithmIdentifier.hpp>

#include <array>
#include <format>
#include <optional>

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
            const auto results = parallel_test_work::run(
                records.size(),
                [&](const std::size_t index) -> std::optional<bool> { return computeRecord(records[index]); });
            for (auto index = std::size_t{}; index < records.size(); ++index) {
                const auto &record = records[index];
                runWithContext(
                    SOURCE_LOCATION(),
                    [&]() -> void {
                        REQUIRE_NOTHROW(results[index].value());
                        const auto &actual = results[index].value();
                        const auto expected = record.value("Result"_el) == "valid"_el;
                        if (actual.has_value()) {
                            REQUIRE_EQUAL(*actual, expected);
                        } else {
                            REQUIRE_FALSE(
                                expected && !record.value(response_key::cFlagsField).contains("SmallPublicKey"_el));
                        }
                    },
                    [&]() -> std::string {
                        return std::format(
                            "{} tcId {} flags {}",
                            el::StringConverter{record.diagnostic()}.toStdString(),
                            el::StringConverter{record.value("Count"_el)}.toStdString(),
                            el::StringConverter{record.value(response_key::cFlagsField)}.toStdString());
                    });
            }
        }
        REQUIRE_EQUAL(recordCount, std::size_t{1080U});
    }

private:
    [[nodiscard]] static auto computeRecord(const CryptologyResponseReader::Record &record) -> std::optional<bool> {
        record.requireAllowedSettings({"Padding"_el, "Hash"_el, "SaltLength"_el, "PublicKeyDer"_el, "AlgorithmDer"_el});
        record.requireAllowedValues({"Count"_el, "Result"_el, response_key::cFlagsField, "Msg"_el, "Sig"_el});
        record.requireValues({"Count"_el, "Result"_el, response_key::cFlagsField, "Msg"_el, "Sig"_el});
        const auto publicKey = PublicKey::fromDerOrThrow(bytesFromHex(record.setting("PublicKeyDer"_el)));
        const auto algorithm = X509AlgorithmIdentifier::fromDerOrThrow(bytesFromHex(record.setting("AlgorithmDer"_el)));
        const auto message = bytesFromHex(record.value("Msg"_el));
        const auto signature = bytesFromHex(record.value("Sig"_el));
        auto actual = false;
        try {
            actual = publicKey.verifySignature(algorithm, message.span(), signature.span());
        } catch (const el::err::ParseError &) {
            return std::nullopt;
        }
        return actual;
    }
};
