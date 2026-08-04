// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyResponseReader.hpp"
#include "CryptologyTestHelper.hpp"

#include <erbsland/cryptology/Hmac.hpp>
#include <erbsland/cryptology/impl/algorithm/Blake2b.hpp>
#include <erbsland/mem/ByteBlock.hpp>

#include <algorithm>
#include <array>
#include <format>
#include <map>
#include <string>
#include <tuple>

using el::cryptology::impl::Blake2b;
using el::mem::ByteBlock;
using namespace el::text::literals;

TESTED_TARGETS(Blake2b Hmac)
class HashPrimitiveFullValidationTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
private:
    void verifyBlake2b(const CryptologyResponseReader::Record &record) {
        record.requireAllowedSettings({"Hash"_el, "Keylen"_el, "Outlen"_el});
        record.requireAllowedValues({"Count"_el, "Len"_el, "Msg"_el, "MD"_el});
        record.requireValues({"Count"_el, "Len"_el, "Msg"_el, "MD"_el});
        REQUIRE_EQUAL(record.setting("Hash"_el), "BLAKE2b"_el);
        REQUIRE_EQUAL(record.unsignedSetting("Keylen"_el), 0U);
        REQUIRE_EQUAL(record.unsignedSetting("Outlen"_el), 512U);
        const auto message = bytesFromHex(record.value("Msg"_el));
        REQUIRE_EQUAL(message.length().toSizeT() * 8U, record.unsignedValue("Len"_el));
        const auto expected = bytesFromHex(record.value("MD"_el));

        auto whole = Blake2b{el::unit::ByteLength{64U}};
        REQUIRE_EQUAL(ByteBlock{whole.digest(message.span())}, expected);

        auto chunked = Blake2b{el::unit::ByteLength{64U}};
        for (auto offset = std::size_t{}; offset < message.length().toSizeT();) {
            const auto count = std::min(((offset * 17U) % 43U) + 1U, message.length().toSizeT() - offset);
            chunked.update(message.span().subspan(offset, count));
            offset += count;
        }
        REQUIRE_EQUAL(ByteBlock{chunked.digest()}, expected);
    }

    void verifyHmac(const CryptologyResponseReader::Record &record, const el::cryptology::HashAlgorithm algorithm) {
        record.requireAllowedSettings({"L"_el});
        record.requireAllowedValues({"Count"_el, "Klen"_el, "Tlen"_el, "Key"_el, "Msg"_el, "Mac"_el});
        record.requireValues({"Count"_el, "Klen"_el, "Tlen"_el, "Key"_el, "Msg"_el, "Mac"_el});
        REQUIRE_EQUAL(record.unsignedSetting("L"_el), algorithm.digestSize().toSizeT());
        const auto key = bytesFromHex(record.value("Key"_el));
        const auto message = bytesFromHex(record.value("Msg"_el));
        const auto expected = bytesFromHex(record.value("Mac"_el));
        REQUIRE_EQUAL(key.length().toSizeT(), record.unsignedValue("Klen"_el));
        REQUIRE_EQUAL(expected.length().toSizeT(), record.unsignedValue("Tlen"_el));

        auto whole = el::cryptology::Hmac{algorithm, key.span()};
        whole.update(message);
        const auto wholeTag = whole.finalize();
        REQUIRE_EQUAL(ByteBlock::fromSpan(wholeTag.span().first(expected.length().toSizeT())), expected);

        const auto firstLength = message.length().toSizeT() / 3U;
        const auto secondLength = (message.length().toSizeT() - firstLength) / 2U;
        auto multipart = el::cryptology::Hmac{algorithm, key.span()};
        multipart.update(message.span().first(firstLength));
        multipart.update(message.span().subspan(firstLength, secondLength));
        multipart.update(message.span().subspan(firstLength + secondLength));
        const auto multipartTag = multipart.finalize();
        REQUIRE_EQUAL(ByteBlock::fromSpan(multipartTag.span().first(expected.length().toSizeT())), expected);
    }

public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testOfficialBlake2bVectors() {
        const auto records = CryptologyResponseReader{"data/cryptology/hash/BLAKE2b.rsp"_el}.read();
        REQUIRE_EQUAL(records.size(), 256U);
        for (const auto &record : records) {
            REQUIRE_EQUAL(record.unsignedValue("Count"_el), record.index);
            runWithContext(
                SOURCE_LOCATION(),
                [&]() -> void { verifyBlake2b(record); },
                [&]() -> std::string { return el::StringConverter{record.diagnostic()}.toStdString(); });
        }
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testNistHmacSha2Vectors() {
        const auto files = std::array{
            std::tuple{
                "data/cryptology/hash/HMAC_SHA256.rsp",
                el::cryptology::HashAlgorithm{el::cryptology::HashAlgorithm::Sha2_256},
                225U},
            std::tuple{
                "data/cryptology/hash/HMAC_SHA384.rsp",
                el::cryptology::HashAlgorithm{el::cryptology::HashAlgorithm::Sha2_384},
                300U}};
        for (const auto &[path, algorithm, expectedRecordCount] : files) {
            const auto records = CryptologyResponseReader{el::String{path}}.read();
            REQUIRE_EQUAL(records.size(), expectedRecordCount);
            auto tagLengthCounts = std::map<std::size_t, std::size_t>{};
            auto keyLengthCounts = std::map<std::size_t, std::size_t>{};
            for (const auto &record : records) {
                runWithContext(
                    SOURCE_LOCATION(),
                    [&]() -> void { verifyHmac(record, algorithm); },
                    [&]() -> std::string { return el::StringConverter{record.diagnostic()}.toStdString(); });
                REQUIRE_EQUAL(record.unsignedValue("Count"_el), record.index);
                ++tagLengthCounts[record.unsignedValue("Tlen"_el)];
                ++keyLengthCounts[record.unsignedValue("Klen"_el)];
            }
            REQUIRE_EQUAL(tagLengthCounts.size(), expectedRecordCount / 75U);
            for (const auto &[tagLength, count] : tagLengthCounts) {
                REQUIRE(tagLength <= algorithm.digestSize().toSizeT());
                REQUIRE_EQUAL(count, 75U);
            }
            REQUIRE_EQUAL(keyLengthCounts.size(), 5U);
            for (const auto &entry : keyLengthCounts) {
                REQUIRE_EQUAL(entry.second, expectedRecordCount / 5U);
            }
        }
    }
};
