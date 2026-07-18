// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringDecoder.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using el::mem::ByteBlock;
using el::text::EncodingErrorMode;
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

    void testBomAndErrorModes() {
        REQUIRE_EQUAL(
            StringConverter{StringDecoder{makeBlock({0xEFU, 0xBBU, 0xBFU, 0x41U})}.toU8String(StringEncoding::Utf8)}
                .toStdString(),
            std::string{"A"});

        const auto malformed = makeBlock({0x41U, 0xC0U, 0x42U});
        REQUIRE_EQUAL(
            StringConverter{StringDecoder{malformed}.toU8String(
                                StringEncoding::Utf8, StringBomMode::Automatic, EncodingErrorMode::Replace)}
                .toStdU32String(),
            std::u32string{U"A\uFFFDB"});
        REQUIRE_EQUAL(
            StringConverter{StringDecoder{malformed}.toU8String(
                                StringEncoding::Utf8, StringBomMode::Automatic, EncodingErrorMode::Ignore)}
                .toStdU32String(),
            std::u32string{U"AB"});
        REQUIRE_THROWS(
            StringDecoder{malformed}.toU8String(
                StringEncoding::Utf8, StringBomMode::Automatic, EncodingErrorMode::Throw));
    }

private:
    [[nodiscard]] static auto makeBlock(const std::vector<uint8_t> &bytes) -> ByteBlock { return ByteBlock{bytes}; }
};
