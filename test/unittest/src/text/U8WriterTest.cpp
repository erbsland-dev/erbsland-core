// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/ByteWriter.hpp>
#include <erbsland/text/StdFormatForText.hpp>
#include <erbsland/text/u8/impl/U8Writer.hpp>
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

TESTED_TARGETS(U8Writer createUtf8String)
class U8WriterTest final : public el::UnitTest {
public:
    void testWriterEncodesCharactersIntoCharBuffer() {
        auto data = std::array<char, 10>{};
        auto writer = el::text::impl::U8Writer{std::span<char>{data}};

        writer.write(Char{U'A'});
        writer.write(Char{0x00A2U});
        writer.write(Char{0x20ACU});
        writer.write(Char{0x1F600U});
        writer.write(Char{0xD800U});
        writer.write(Char{0xFEFFU});
        writer.write(Char::endOfData());
        writer.write(Char::byteOrderMark());

        const auto written = std::string{data.data(), data.size()};

        REQUIRE_EQUAL(writer.position(), std::size_t{10});
        REQUIRE_EQUAL(written, th::stdStringFromHex("41 C2 A2 E2 82 AC F0 9F 98 80"));
    }

    void testWriterEncodesCharactersIntoChar8Buffer() {
        auto data = std::array<char8_t, 4>{};
        auto writer = el::text::impl::U8Writer{std::span<char8_t>{data}};

        writer.write(Char{0x1F600U});

        const auto written = std::u8string{data.data(), data.size()};

        REQUIRE_EQUAL(writer.position(), std::size_t{4});
        REQUIRE_EQUAL(written, std::u8string{u8"😀"});
    }

    void testWriterEncodesCharactersIntoByteWriter() {
        auto byteWriter = ByteWriter{};
        auto writer = el::text::impl::U8Writer{byteWriter};

        writer.writeBom();
        writer.write(Char{U'A'});
        writer.write(Char{0x1F600U});

        REQUIRE_EQUAL(writer.position(), std::size_t{8});
        REQUIRE_EQUAL(
            byteWriter.toByteBlock().toUInt8Vector(),
            std::vector<uint8_t>({0xEFU, 0xBBU, 0xBFU, 0x41U, 0xF0U, 0x9FU, 0x98U, 0x80U}));
    }

    void testWriterStopsAtBufferEndWithoutOverflow() {
        auto data = std::array<char, 3>{'x', 'x', 'x'};
        auto writer = el::text::impl::U8Writer{std::span<char>{data}};

        writer.write(Char{0x1F600U});

        const auto written = std::string{data.data(), data.size()};

        REQUIRE_EQUAL(writer.position(), std::size_t{3});
        REQUIRE_EQUAL(written, th::stdStringFromHex("F0 9F 98"));
    }

    void testCreateUtf8StringReturnsNormalizedUtf8() {
        const auto malformed = th::stdStringFromHex("41 C0 80 42");

        REQUIRE_EQUAL(
            th::toStdU32String(
                el::text::impl::createUtf8String<std::string>(
                    std::span<const char>{malformed.data(), malformed.size()})),
            std::u32string{U"A\uFFFD\uFFFDB"});
        REQUIRE_EQUAL(
            th::toStdU32String(
                el::text::impl::createUtf8String<std::u8string>(
                    std::span<const char>{malformed.data(), malformed.size()})),
            std::u32string{U"A\uFFFD\uFFFDB"});
    }

    void testCreateUtf8StringOverAllMalformedUtf8Categories() {
        for (const auto error : th::allUtf8Errors) {
            WITH_CONTEXT(requireCreateUtf8StringReplacement(error));
        }
    }

private:
    void requireCreateUtf8StringReplacement(const th::Utf8Error error) {
        const auto malformed = th::invalidUtf8(error, "A", "B");

        REQUIRE_EQUAL(
            th::toStdU32String(
                el::text::impl::createUtf8String<std::string>(
                    std::span<const char>{malformed.data(), malformed.size()})),
            expectedReplaceDecodedText(error));
        REQUIRE_EQUAL(
            th::toStdU32String(
                el::text::impl::createUtf8String<std::u8string>(
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
