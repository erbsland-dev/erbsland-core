// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/text/EncodingError.hpp>
#include <erbsland/text/StdFormat.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringDecoder.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using el::mem::ByteBlock;
using el::text::EncodingMode;
using el::text::StringBomMode;
using el::text::StringConverter;
using el::text::StringDecoder;
using el::text::StringEncoding;

static_assert(std::is_same_v<decltype(std::declval<StringDecoder &>().decode(StringEncoding::Utf8)), el::text::String>);
static_assert(
    std::is_same_v<decltype(std::declval<StringDecoder &>().toU16String(StringEncoding::Utf16)), el::text::U16String>);

TESTED_TARGETS(StringDecoder)
class StringDecoderTest final : public el::UnitTest {
public:
    void testDecodeUtf8Utf16AndUtf32() {
        REQUIRE_EQUAL(
            StringConverter{StringDecoder{makeBlock({0x41U, 0xC2U, 0xA2U})}.decode(StringEncoding::Utf8)}.toStdString(),
            std::string{"A\302\242"});
        REQUIRE_EQUAL(
            StringConverter{
                StringDecoder{makeBlock({0x41U, 0x00U, 0xA2U, 0x00U})}.toU16String(StringEncoding::Utf16LittleEndian)}
                .toStdU16String(),
            std::u16string{u"A¢"});
        REQUIRE_EQUAL(
            StringConverter{
                StringDecoder{makeBlock({0x41U, 0x00U, 0x00U, 0x00U})}.toU32String(StringEncoding::Utf32LittleEndian)}
                .toStdU32String(),
            std::u32string{U"A"});
    }

    void testBomAndEncodingModes() {
        REQUIRE_EQUAL(
            StringConverter{StringDecoder{makeBlock({0xEFU, 0xBBU, 0xBFU, 0x41U})}.toU8String(StringEncoding::Utf8)}
                .toStdString(),
            std::string{"A"});

        const auto malformed = makeBlock({0x41U, 0xC0U, 0x42U});
        REQUIRE_EQUAL(
            StringConverter{StringDecoder{malformed}.toU8String(
                                StringEncoding::Utf8, StringBomMode::Automatic, EncodingMode::Tolerant)}
                .toStdU32String(),
            std::u32string{U"A\uFFFDB"});
        REQUIRE_THROWS(
            StringDecoder{malformed}.toU8String(StringEncoding::Utf8, StringBomMode::Automatic, EncodingMode::Strict));
    }

    void testInitialAndRepeatedBomForEveryEncoding() {
        struct TestCase final {
            StringEncoding encoding;
            std::vector<uint8_t> bom;
            std::vector<uint8_t> letterA;
            std::vector<uint8_t> letterB;
        };
        const auto testCases = std::vector<TestCase>{
            {StringEncoding::Utf8, {0xEFU, 0xBBU, 0xBFU}, {0x41U}, {0x42U}},
            {StringEncoding::Utf16LittleEndian, {0xFFU, 0xFEU}, {0x41U, 0x00U}, {0x42U, 0x00U}},
            {StringEncoding::Utf16BigEndian, {0xFEU, 0xFFU}, {0x00U, 0x41U}, {0x00U, 0x42U}},
            {StringEncoding::Utf32LittleEndian,
                {0xFFU, 0xFEU, 0x00U, 0x00U},
                {0x41U, 0x00U, 0x00U, 0x00U},
                {0x42U, 0x00U, 0x00U, 0x00U}},
            {StringEncoding::Utf32BigEndian,
                {0x00U, 0x00U, 0xFEU, 0xFFU},
                {0x00U, 0x00U, 0x00U, 0x41U},
                {0x00U, 0x00U, 0x00U, 0x42U}},
        };

        for (const auto &testCase : testCases) {
            auto initial = testCase.bom;
            initial.insert(initial.end(), testCase.letterA.begin(), testCase.letterA.end());
            REQUIRE_EQUAL(
                StringConverter{StringDecoder{makeBlock(initial)}.decode(
                                    testCase.encoding, StringBomMode::Automatic, EncodingMode::Strict)}
                    .toStdU32String(),
                std::u32string{U"A"});
            REQUIRE_EQUAL(
                StringConverter{StringDecoder{makeBlock(initial)}.decode(
                                    testCase.encoding, StringBomMode::Require, EncodingMode::Strict)}
                    .toStdU32String(),
                std::u32string{U"A"});
            REQUIRE_THROWS_AS(
                el::text::EncodingError,
                StringDecoder{makeBlock(initial)}.decode(
                    testCase.encoding, StringBomMode::Reject, EncodingMode::Strict));

            auto repeated = testCase.bom;
            repeated.insert(repeated.end(), testCase.bom.begin(), testCase.bom.end());
            repeated.insert(repeated.end(), testCase.letterA.begin(), testCase.letterA.end());
            REQUIRE_EQUAL(
                StringConverter{StringDecoder{makeBlock(repeated)}.decode(
                                    testCase.encoding, StringBomMode::Automatic, EncodingMode::Tolerant)}
                    .toStdU32String(),
                std::u32string{U"\uFFFDA"});
            REQUIRE_THROWS_AS(
                el::text::EncodingError,
                StringDecoder{makeBlock(repeated)}.decode(
                    testCase.encoding, StringBomMode::Automatic, EncodingMode::Strict));

            auto embedded = testCase.letterA;
            embedded.insert(embedded.end(), testCase.bom.begin(), testCase.bom.end());
            embedded.insert(embedded.end(), testCase.letterB.begin(), testCase.letterB.end());
            REQUIRE_EQUAL(
                StringConverter{StringDecoder{makeBlock(embedded)}.decode(
                                    testCase.encoding, StringBomMode::Reject, EncodingMode::Tolerant)}
                    .toStdU32String(),
                std::u32string{U"A\uFFFDB"});
            REQUIRE_THROWS_AS(
                el::text::EncodingError,
                StringDecoder{makeBlock(embedded)}.decode(
                    testCase.encoding, StringBomMode::Reject, EncodingMode::Strict));
        }
    }

private:
    [[nodiscard]] static auto makeBlock(const std::vector<uint8_t> &bytes) -> ByteBlock {
        return ByteBlock::fromVector(bytes);
    }
};
