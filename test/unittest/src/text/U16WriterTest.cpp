// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/ByteWriter.hpp>
#include <erbsland/text/u16/impl/U16Writer.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

using el::mem::ByteWriter;
using el::text::Char;

namespace th = erbsland::unittest::th;

TESTED_TARGETS(U16Writer createUtf16String)
class U16WriterTest final : public el::UnitTest {
public:
    void testWriterEncodesCharactersIntoChar16Buffer() {
        auto data = std::array<char16_t, 4>{};
        auto writer = el::text::impl::U16Writer{std::span<char16_t>{data}};

        writer.write(Char{U'A'});
        writer.write(Char{0x20ACU});
        writer.write(Char{0x1F600U});
        writer.write(Char{0xD800U});

        const auto written = std::u16string{data.data(), data.size()};

        REQUIRE_EQUAL(writer.position(), std::size_t{4});
        REQUIRE_EQUAL(written, th::stdU16StringFromHex("0041 20AC D83D DE00"));
    }

    void testWriterStopsAtBufferEndWithoutOverflow() {
        auto data = std::array<char16_t, 1>{};
        auto writer = el::text::impl::U16Writer{std::span<char16_t>{data}};

        writer.write(Char{0x1F600U});

        const auto written = std::u16string{data.data(), data.size()};

        REQUIRE_EQUAL(writer.position(), std::size_t{1});
        REQUIRE_EQUAL(written, th::stdU16StringFromHex("D83D"));
    }

    void testWriterEncodesCharactersIntoByteWriter() {
        auto byteWriter = ByteWriter{};
        auto writer = el::text::impl::U16Writer{byteWriter};

        writer.writeBom();
        writer.write(Char{U'A'});
        writer.write(Char{0x1F600U});

        REQUIRE_EQUAL(writer.position(), std::size_t{8});
        REQUIRE_EQUAL(
            byteWriter.toByteBlock().toUInt8Vector(),
            std::vector<uint8_t>({0xFFU, 0xFEU, 0x41U, 0x00U, 0x3DU, 0xD8U, 0x00U, 0xDEU}));
    }

    void testCreateUtf16StringReturnsNormalizedText() {
        const auto malformed = th::stdStringFromHex("41 C0 80 42");

        REQUIRE_EQUAL(
            th::toStdU32String(
                el::text::impl::createUtf16String<std::u16string>(
                    std::span<const char>{malformed.data(), malformed.size()})),
            std::u32string{U"A\uFFFD\uFFFDB"});
#ifdef ERBSLAND_WCHAR_16BIT
        REQUIRE_EQUAL(
            th::toStdU32String(
                el::text::impl::createUtf16String<std::wstring>(
                    std::span<const char>{malformed.data(), malformed.size()})),
            std::u32string{U"A\uFFFD\uFFFDB"});
#endif
    }

    void testCreateUtf16StringOverAllMalformedUtf8Categories() {
        for (const auto error : th::allUtf8Errors) {
            WITH_CONTEXT(requireCreateUtf16StringReplacement(error));
        }
    }

private:
    void requireCreateUtf16StringReplacement(const th::Utf8Error error) {
        const auto malformed = th::invalidUtf8(error, "A", "B");

        REQUIRE_EQUAL(
            th::toStdU32String(
                el::text::impl::createUtf16String<std::u16string>(
                    std::span<const char>{malformed.data(), malformed.size()})),
            expectedReplaceDecodedText(error));
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
