// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/RingBuffer.hpp>
#include <erbsland/text/EncodingError.hpp>
#include <erbsland/text/StdFormatForText.hpp>
#include <erbsland/text/StringEncoder.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/text/u16/U16StringEditor.hpp>
#include <erbsland/text/u32/U32StringEditor.hpp>
#include <erbsland/text/u8/U8StringEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>
#include <span>
#include <string>
#include <vector>

using el::unit::CpIndex;
using el::unit::CpLength;
using el::unit::CpRange;
using namespace el::text;

TESTED_TARGETS(StringEncoder StringEncoderTraits)
class StringEncoderTest final : public el::UnitTest {
private:
    template <typename T>
    void requireLengthMatches(const T &text) {
        for (
            const auto encodingValue :
            {StringEncoding::Utf8,
                StringEncoding::Utf16,
                StringEncoding::Utf16LittleEndian,
                StringEncoding::Utf16BigEndian,
                StringEncoding::Utf32,
                StringEncoding::Utf32LittleEndian,
                StringEncoding::Utf32BigEndian}) {
            const auto encoding = StringEncoding{encodingValue};
            for (const auto bomMode : {StringBomMode::Automatic, StringBomMode::Require, StringBomMode::Reject}) {
                const auto encoder = StringEncoder{text};
                for (
                    const auto errorMode :
                    {EncodingErrorMode::Replace, EncodingErrorMode::Ignore, EncodingErrorMode::Throw}) {
                    REQUIRE_EQUAL(
                        encoder.encodedLength(encoding, bomMode, errorMode),
                        encoder.encode(encoding, bomMode, errorMode).length());
                }
            }
        }
    }

    template <typename T>
    void requireDirectEncodingMatches(const T &text) {
        for (
            const auto encodingValue :
            {StringEncoding::Utf8,
                StringEncoding::Utf16,
                StringEncoding::Utf16LittleEndian,
                StringEncoding::Utf16BigEndian,
                StringEncoding::Utf32,
                StringEncoding::Utf32LittleEndian,
                StringEncoding::Utf32BigEndian}) {
            const auto encoding = StringEncoding{encodingValue};
            for (const auto bomMode : {StringBomMode::Automatic, StringBomMode::Require, StringBomMode::Reject}) {
                auto buffer = el::mem::RingBuffer{el::unit::ByteLength{4U}, el::unit::ByteLength{128U}};
                const auto encoder = StringEncoder{text};
                REQUIRE(isSuccessful(encoder.encodeTo(buffer, encoding, bomMode)));
                REQUIRE_EQUAL(buffer.read(el::unit::ByteLength::infinite()), encoder.encode(encoding, bomMode));
            }
        }
    }

public:
    void testEncodeUtf8WithoutBom() {
        const auto text = U8StringEditor{std::u8string_view{u8"A¢"}};

        REQUIRE_EQUAL(
            StringEncoder{text}.encode(StringEncoding::Utf8, StringBomMode::Reject).toUInt8Vector(),
            std::vector<uint8_t>({0x41U, 0xC2U, 0xA2U}));
    }

    void testEncodeUtf16ViewWithRequiredBom() {
        const auto text = U16StringEditor{std::u16string_view{u"A¢"}};

        REQUIRE_EQUAL(
            StringEncoder{U16String{text}}
                .encode(StringEncoding::Utf16LittleEndian, StringBomMode::Require)
                .toUInt8Vector(),
            std::vector<uint8_t>({0xFFU, 0xFEU, 0x41U, 0x00U, 0xA2U, 0x00U}));
    }

    void testEncodeUtf32AndCodePoint() {
        const auto text = U32StringEditor{std::u32string_view{U"A¢"}};
        const auto charView = U8StringEditor{std::u8string_view{u8"xxA¢yy"}}.slice(CpRange{CpIndex{2U}, CpLength{2U}});

        REQUIRE_EQUAL(
            StringEncoder{text}.encode(StringEncoding::Utf32LittleEndian, StringBomMode::Reject).toUInt8Vector(),
            std::vector<uint8_t>({0x41U, 0x00U, 0x00U, 0x00U, 0xA2U, 0x00U, 0x00U, 0x00U}));
        REQUIRE_EQUAL(
            StringEncoder{charView}.encode(StringEncoding::Utf8, StringBomMode::Reject).toUInt8Vector(),
            std::vector<uint8_t>({0x41U, 0xC2U, 0xA2U}));
    }

    void testEncodedLengthForAllSourceWidths() {
        const auto u8Editor = U8StringEditor{std::u8string_view{u8"A—😀"}};
        const auto u16Editor = U16StringEditor{std::u16string_view{u"A—😀"}};
        const auto u32Editor = U32StringEditor{std::u32string_view{U"A—😀"}};

        WITH_CONTEXT(requireLengthMatches(u8Editor));
        WITH_CONTEXT(requireLengthMatches(U8String{u8Editor}));
        WITH_CONTEXT(requireLengthMatches(u16Editor));
        WITH_CONTEXT(requireLengthMatches(U16String{u16Editor}));
        WITH_CONTEXT(requireLengthMatches(u32Editor));
        WITH_CONTEXT(requireLengthMatches(U32String{u32Editor}));
    }

    void testEncodeToMatchesByteBlockEncoding() {
        const auto u8Editor = U8StringEditor{std::u8string_view{u8"A—😀"}};
        const auto u16Editor = U16StringEditor{std::u16string_view{u"A—😀"}};
        const auto u32Editor = U32StringEditor{std::u32string_view{U"A—😀"}};

        WITH_CONTEXT(requireDirectEncodingMatches(u8Editor));
        WITH_CONTEXT(requireDirectEncodingMatches(U8String{u8Editor}));
        WITH_CONTEXT(requireDirectEncodingMatches(u16Editor));
        WITH_CONTEXT(requireDirectEncodingMatches(U16String{u16Editor}));
        WITH_CONTEXT(requireDirectEncodingMatches(u32Editor));
        WITH_CONTEXT(requireDirectEncodingMatches(U32String{u32Editor}));
    }

    void testEncodeToWraps() {
        auto buffer = el::mem::RingBuffer{el::unit::ByteLength{16U}, el::unit::ByteLength{32U}};
        const auto prefix = std::vector<el::mem::Byte>(14U, el::mem::Byte{0xaaU});
        REQUIRE(isSuccessful(buffer.writeExact(prefix)));
        static_cast<void>(buffer.read(el::unit::ByteLength{12U}));

        const auto text = U8StringEditor{std::u8string_view{u8"A—😀"}};
        const auto encoder = StringEncoder{text};
        REQUIRE(isSuccessful(encoder.encodeTo(buffer, StringEncoding::Utf8, StringBomMode::Reject)));

        const auto result = buffer.read(el::unit::ByteLength::infinite()).toUInt8Vector();
        auto expected = std::vector<uint8_t>({0xaaU, 0xaaU});
        const auto encoded = encoder.encode(StringEncoding::Utf8, StringBomMode::Reject).toUInt8Vector();
        expected.insert(expected.end(), encoded.begin(), encoded.end());
        REQUIRE_EQUAL(result, expected);
    }

    void testEncodeToGrows() {
        const auto text = U8StringEditor{std::u8string_view{u8"A—😀"}};
        auto buffer = el::mem::RingBuffer{el::unit::ByteLength{4U}, el::unit::ByteLength{32U}};

        REQUIRE(isSuccessful(StringEncoder{text}.encodeTo(buffer, StringEncoding::Utf8, StringBomMode::Require)));
        REQUIRE_GREATER(buffer.capacity(), el::unit::ByteLength{4U});
    }

    void testEncodeToCapacityFailureIsAtomic() {
        auto buffer = el::mem::RingBuffer{el::unit::ByteLength{4U}};
        const auto prefix = std::vector<el::mem::Byte>({el::mem::Byte{0xaaU}});
        REQUIRE(isSuccessful(buffer.writeExact(prefix)));

        const auto text = U8StringEditor{std::u8string_view{u8"A—"}};
        REQUIRE(isFailure(StringEncoder{text}.encodeTo(buffer, StringEncoding::Utf8, StringBomMode::Reject)));
        REQUIRE_EQUAL(buffer.read(el::unit::ByteLength::infinite()).toUInt8Vector(), std::vector<uint8_t>({0xaaU}));
    }

    void testErrorModesAndExceptionAtomicity() {
        const auto malformed = U32StringEditor{std::u32string{char32_t{0x110000U}, U'A'}};
        const auto encoder = StringEncoder{malformed};
        for (const auto errorMode : {EncodingErrorMode::Replace, EncodingErrorMode::Ignore}) {
            auto buffer = el::mem::RingBuffer{el::unit::ByteLength{16U}};
            REQUIRE(isSuccessful(encoder.encodeTo(buffer, StringEncoding::Utf8, StringBomMode::Reject, errorMode)));
            REQUIRE_EQUAL(
                buffer.read(el::unit::ByteLength::infinite()),
                encoder.encode(StringEncoding::Utf8, StringBomMode::Reject, errorMode));
        }

        auto buffer = el::mem::RingBuffer{el::unit::ByteLength{16U}};
        const auto prefix = std::vector<el::mem::Byte>({el::mem::Byte{0xaaU}});
        REQUIRE(isSuccessful(buffer.writeExact(prefix)));
        REQUIRE_THROWS_AS(
            EncodingError,
            encoder.encodeTo(buffer, StringEncoding::Utf8, StringBomMode::Reject, EncodingErrorMode::Throw));
        REQUIRE_EQUAL(buffer.read(el::unit::ByteLength::infinite()).toUInt8Vector(), std::vector<uint8_t>({0xaaU}));
    }

    void testRawBomCodePointIsInvalidContent() {
        const auto text = U32StringEditor{std::u32string{U'A', char32_t{0xFEFFU}, U'B'}};
        const auto encoder = StringEncoder{text};

        REQUIRE_EQUAL(
            encoder.encode(StringEncoding::Utf8, StringBomMode::Reject, EncodingErrorMode::Replace).toUInt8Vector(),
            std::vector<uint8_t>({0x41U, 0xEFU, 0xBFU, 0xBDU, 0x42U}));
        REQUIRE_EQUAL(
            encoder.encode(StringEncoding::Utf8, StringBomMode::Reject, EncodingErrorMode::Ignore).toUInt8Vector(),
            std::vector<uint8_t>({0x41U, 0x42U}));
        REQUIRE_THROWS_AS(
            EncodingError, encoder.encode(StringEncoding::Utf8, StringBomMode::Reject, EncodingErrorMode::Throw));
    }

    void testStandaloneCallsApplyBomIndependently() {
        const auto text = U8StringEditor{std::u8string_view{u8"A"}};
        const auto encoder = StringEncoder{text};
        auto buffer = el::mem::RingBuffer{el::unit::ByteLength{16U}};

        REQUIRE(isSuccessful(encoder.encodeTo(buffer, StringEncoding::Utf16, StringBomMode::Require)));
        REQUIRE(isSuccessful(encoder.encodeTo(buffer, StringEncoding::Utf16, StringBomMode::Require)));
        REQUIRE_EQUAL(
            buffer.read(el::unit::ByteLength::infinite()).toUInt8Vector(),
            std::vector<uint8_t>({0xffU, 0xfeU, 0x41U, 0x00U, 0xffU, 0xfeU, 0x41U, 0x00U}));
    }

    void testEmptyTextCanProduceBomOnly() {
        const auto encoder = StringEncoder{U8StringEditor{}};
        auto buffer = el::mem::RingBuffer{el::unit::ByteLength{4U}};

        REQUIRE_EQUAL(
            encoder.encode(StringEncoding::Utf16BigEndian, StringBomMode::Automatic).toUInt8Vector(),
            std::vector<uint8_t>({0xfeU, 0xffU}));
        REQUIRE_EQUAL(
            encoder.encodedLength(StringEncoding::Utf16BigEndian, StringBomMode::Automatic), el::unit::ByteLength{2U});
        REQUIRE(isSuccessful(encoder.encodeTo(buffer, StringEncoding::Utf16BigEndian, StringBomMode::Automatic)));
        REQUIRE_EQUAL(
            buffer.read(el::unit::ByteLength::infinite()).toUInt8Vector(), std::vector<uint8_t>({0xfeU, 0xffU}));
        REQUIRE(encoder.encode(StringEncoding::Utf8, StringBomMode::Automatic).isEmpty());
    }

    void testExplicitBomSignaturesForEveryEncoding() {
        const auto encoder = StringEncoder{U8StringEditor{}};
        REQUIRE_EQUAL(
            encoder.encode(StringEncoding::Utf8, StringBomMode::Require).toUInt8Vector(),
            std::vector<uint8_t>({0xEFU, 0xBBU, 0xBFU}));
        REQUIRE_EQUAL(
            encoder.encode(StringEncoding::Utf16LittleEndian, StringBomMode::Require).toUInt8Vector(),
            std::vector<uint8_t>({0xFFU, 0xFEU}));
        REQUIRE_EQUAL(
            encoder.encode(StringEncoding::Utf16BigEndian, StringBomMode::Require).toUInt8Vector(),
            std::vector<uint8_t>({0xFEU, 0xFFU}));
        REQUIRE_EQUAL(
            encoder.encode(StringEncoding::Utf32LittleEndian, StringBomMode::Require).toUInt8Vector(),
            std::vector<uint8_t>({0xFFU, 0xFEU, 0x00U, 0x00U}));
        REQUIRE_EQUAL(
            encoder.encode(StringEncoding::Utf32BigEndian, StringBomMode::Require).toUInt8Vector(),
            std::vector<uint8_t>({0x00U, 0x00U, 0xFEU, 0xFFU}));
    }
};
