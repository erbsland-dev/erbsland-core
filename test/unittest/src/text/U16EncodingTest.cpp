// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/StdFormat.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/u16/impl/U16Encoding.hpp>
#include <erbsland/text/u8/impl/U8StringReadTools.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>
#include <string_view>

using el::text::Char;
using el::text::EncodingMode;
using el::unit::ByteRange;

namespace th = erbsland::unittest::th;

TESTED_TARGETS(U16Encoding)
class U16EncodingTest final : public el::UnitTest {
public:
    void testEncodedLength() {
        REQUIRE_EQUAL(el::text::impl::utf16::encodedLength(Char{U'A'}).toSizeT(), std::size_t{1});
        REQUIRE_EQUAL(el::text::impl::utf16::encodedLength(Char{0x20ACU}).toSizeT(), std::size_t{1});
        REQUIRE_EQUAL(el::text::impl::utf16::encodedLength(Char{0x1F600U}).toSizeT(), std::size_t{2});
        REQUIRE_EQUAL(el::text::impl::utf16::encodedLength(Char{0xD800U}).toSizeT(), std::size_t{1});
    }

    void testForEachDecodedCharacterPreservesValidSequences() {
        const auto text = th::stdU16StringFromHex("0041 00A2 20AC D83D DE00");
        auto result = std::u32string{};

        el::text::impl::utf16::forEachDecodedCharacter(text, EncodingMode::Strict, [&](const Char character) -> void {
            result.push_back(character.toRawValue());
        });

        REQUIRE_EQUAL(result, std::u32string{U"A¢€😀"});
    }

    void testUtf16TolerantModeHandlesInvalidSequences() {
        const auto text = th::stdU16StringFromHex("D800 0041 FEFF DC00 D800 D83D DE00");

        REQUIRE_EQUAL(
            collectDecodedCharacters(text, EncodingMode::Tolerant), std::u32string{U"\uFFFDA\uFFFD\uFFFD\uFFFD😀"});
    }

    void testUtf16StrictModeRejectsAllInvalidCases() {
        REQUIRE_THROWS(
            el::text::impl::utf16::forEachDecodedCharacter(
                th::stdU16StringFromHex("D800"), EncodingMode::Strict, [&](const Char) -> void {}));
        REQUIRE_THROWS(
            el::text::impl::utf16::forEachDecodedCharacter(
                th::stdU16StringFromHex("DC00"), EncodingMode::Strict, [&](const Char) -> void {}));
        REQUIRE_THROWS(
            el::text::impl::utf16::forEachDecodedCharacter(
                th::stdU16StringFromHex("D800 0041"), EncodingMode::Strict, [&](const Char) -> void {}));
        REQUIRE_THROWS(
            el::text::impl::utf16::forEachDecodedCharacter(
                th::stdU16StringFromHex("0041 D800"), EncodingMode::Strict, [&](const Char) -> void {}));
        REQUIRE_THROWS(
            el::text::impl::utf16::forEachDecodedCharacter(
                th::stdU16StringFromHex("FEFF"), EncodingMode::Strict, [&](const Char) -> void {}));
    }

    void testToUtf16StringFromUtf8OverAllMalformedCategories() {
        for (const auto error : th::allUtf8Errors) {
            WITH_CONTEXT(requireMalformedUtf8Replacement(error));
        }
    }

private:
    [[nodiscard]] static auto collectDecodedCharacters(const std::u16string_view text, const EncodingMode mode)
        -> std::u32string {
        auto result = std::u32string{};
        el::text::impl::utf16::forEachDecodedCharacter(
            text, mode, [&](const Char character) -> void { result.push_back(character.toRawValue()); });
        return result;
    }

    void requireMalformedUtf8Replacement(const th::Utf8Error error) {
        const auto malformed = th::invalidUtf8(error, "A", "B");
        const auto converted =
            el::text::impl::U8StringReadTools{
                el::text::impl::U8StringDataView{
                    std::span<const char>{malformed.data(), malformed.size()}, ByteRange::fromSizeT(malformed.size())}}
                .toStdU16String();

        REQUIRE_EQUAL(th::toStdU32String(converted), expectedReplaceDecodedText(error));
    }

    [[nodiscard]] static auto expectedReplaceDecodedText(const th::Utf8Error error) -> std::u32string {
        switch (error) {
        case th::Utf8Error::UnexpectedContinuationByte:
        case th::Utf8Error::Truncated2ByteSequence:
        case th::Utf8Error::SurrogateCodePoint:
        case th::Utf8Error::CodePointBeyondUnicodeRange:
        case th::Utf8Error::InvalidStartByte:
            return std::u32string{U"A\uFFFDB"};
        case th::Utf8Error::Overlong2ByteSequence:
            return std::u32string{U"A\uFFFD\uFFFDB"};
        case th::Utf8Error::InvalidContinuationByteIn2ByteSequence:
            return std::u32string{U"A\uFFFD B"};
        case th::Utf8Error::Overlong3ByteSequence:
            return std::u32string{U"A\uFFFD\uFFFD\uFFFDB"};
        case th::Utf8Error::Truncated3ByteSequence:
            return std::u32string{U"A\uFFFD\uFFFDB"};
        case th::Utf8Error::InvalidContinuationByteIn3ByteSequence:
            return std::u32string{U"A\uFFFD\uFFFD B"};
        case th::Utf8Error::Overlong4ByteSequence:
            return std::u32string{U"A\uFFFD\uFFFD\uFFFD\uFFFDB"};
        case th::Utf8Error::Truncated4ByteSequence:
            return std::u32string{U"A\uFFFD\uFFFD\uFFFDB"};
        case th::Utf8Error::InvalidContinuationByteIn4ByteSequence:
            return std::u32string{U"A\uFFFD\uFFFD\uFFFD B"};
        default:
            return {};
        }
    }
};
