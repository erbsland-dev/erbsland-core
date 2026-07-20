// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/ByteWriter.hpp>
#include <erbsland/text/StdFormatForText.hpp>
#include <erbsland/text/u32/impl/U32Writer.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>
#include <span>
#include <string>
#include <vector>

using el::mem::ByteWriter;
using el::text::Char;

namespace th = erbsland::unittest::th;

TESTED_TARGETS(U32Writer createUtf32String)
class U32WriterTest final : public el::UnitTest {
public:
    void testWriterEncodesCharactersIntoByteWriter() {
        auto byteWriter = ByteWriter{};
        auto writer = el::text::impl::U32Writer{byteWriter};

        writer.write(Char{0xFEFFU});
        writer.write(Char::endOfData());
        writer.write(Char::byteOrderMark());
        writer.writeBom();
        writer.write(Char{U'A'});
        writer.write(Char{0x1F600U});

        REQUIRE_EQUAL(writer.position(), std::size_t{12});
        REQUIRE_EQUAL(
            byteWriter.toByteBlock().toUInt8Vector(),
            std::vector<uint8_t>({0xFFU, 0xFEU, 0x00U, 0x00U, 0x41U, 0x00U, 0x00U, 0x00U, 0x00U, 0xF6U, 0x01U, 0x00U}));
    }

    void testCreateUtf32StringReturnsNormalizedText() {
        const auto malformed = th::stdStringFromHex("41 C0 80 42");

        REQUIRE_EQUAL(
            el::text::impl::createUtf32String<std::u32string>(
                std::span<const char>{malformed.data(), malformed.size()}),
            std::u32string{U"A\uFFFD\uFFFDB"});
#ifndef ERBSLAND_WCHAR_16BIT
        REQUIRE_EQUAL(
            th::toStdU32String(
                el::text::impl::createUtf32String<std::wstring>(
                    std::span<const char>{malformed.data(), malformed.size()})),
            std::u32string{U"A\uFFFD\uFFFDB"});
#endif
    }

    void testCreateUtf32StringOverAllMalformedUtf8Categories() {
        for (const auto error : th::allUtf8Errors) {
            const auto malformed = th::invalidUtf8(error, "A", "B");
            REQUIRE_EQUAL(
                el::text::impl::createUtf32String<std::u32string>(
                    std::span<const char>{malformed.data(), malformed.size()}),
                expectedReplaceDecodedText(error));
        }
    }

private:
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
