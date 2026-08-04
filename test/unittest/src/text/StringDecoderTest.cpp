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
static_assert(std::is_same_v<decltype(std::declval<StringDecoder &>().validateOrThrow(StringEncoding::Utf8)), void>);

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

    void testValidateEncodedDataWithoutDecoding() {
        const auto utf8 = StringDecoder{makeBlock({0x41U, 0xC2U, 0xA2U})};
        REQUIRE_NOTHROW(utf8.validateOrThrow(StringEncoding::Utf8));

        const auto malformedUtf8 = StringDecoder{makeBlock({0x41U, 0xC0U, 0x42U})};
        REQUIRE_THROWS_AS(el::text::EncodingError, malformedUtf8.validateOrThrow(StringEncoding::Utf8));

        const auto utf8Bom = StringDecoder{makeBlock({0xEFU, 0xBBU, 0xBFU, 0x41U})};
        REQUIRE_NOTHROW(utf8Bom.validateOrThrow(StringEncoding::Utf8, StringBomMode::Require));
        REQUIRE_THROWS_AS(
            el::text::EncodingError, utf8Bom.validateOrThrow(StringEncoding::Utf8, StringBomMode::Reject));
        REQUIRE_THROWS_AS(el::text::EncodingError, utf8.validateOrThrow(StringEncoding::Utf8, StringBomMode::Require));

        const auto utf16LittleEndian = StringDecoder{makeBlock({0x41U, 0x00U, 0x3DU, 0xD8U, 0x00U, 0xDEU})};
        REQUIRE_NOTHROW(utf16LittleEndian.validateOrThrow(StringEncoding::Utf16LittleEndian));
        const auto utf16BigEndian = StringDecoder{makeBlock({0x00U, 0x41U, 0xD8U, 0x3DU, 0xDEU, 0x00U})};
        REQUIRE_NOTHROW(utf16BigEndian.validateOrThrow(StringEncoding::Utf16BigEndian));
        REQUIRE_THROWS_AS(
            el::text::EncodingError,
            StringDecoder{makeBlock({0x41U})}.validateOrThrow(StringEncoding::Utf16LittleEndian));

        const auto utf32BigEndian = StringDecoder{makeBlock({0x00U, 0x01U, 0xF6U, 0x00U})};
        REQUIRE_NOTHROW(utf32BigEndian.validateOrThrow(StringEncoding::Utf32BigEndian));
        REQUIRE_THROWS_AS(
            el::text::EncodingError,
            StringDecoder{makeBlock({0x00U, 0x00U, 0xD8U, 0x00U})}.validateOrThrow(StringEncoding::Utf32BigEndian));
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
