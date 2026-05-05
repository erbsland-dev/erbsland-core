// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/mem/ByteBlockView.hpp>
#include <erbsland/mem/ByteReader.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringDecoder.hpp>
#include <erbsland/text/StringEncoder.hpp>
#include <erbsland/text/u16/impl/U16Encoding.hpp>
#include <erbsland/text/u32/impl/U32Encoding.hpp>
#include <erbsland/text/u8/impl/U8Encoding.hpp>
#include <erbsland/text/u8/impl/U8StringEncodingTools.hpp>
#include <erbsland/text/u8/U8String.hpp>
#include <erbsland/text/u8/U8StringCharView.hpp>
#include <erbsland/text/u8/U8StringView.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>
#include <vector>

using el::mem::ByteBlock;
using el::mem::ByteReader;
using el::mem::Endianness;
using namespace el::text;
using namespace el::unit;

TESTED_TARGETS(U8String U8StringView U8StringCharView U8StringEncodingTools StringEncoding StringBomMode)
class U8StringEncodingTest final : public el::UnitTest {
public:
    void testEncodeUtf8AndBomModes() {
        const auto text = sampleText();

        REQUIRE_EQUAL(StringEncoder{text}.encode(StringEncoding::Utf8).toUInt8Vector(), utf8Bytes());
        REQUIRE_EQUAL(
            StringEncoder{text}.encode(StringEncoding::Utf8, StringBomMode::Reject).toUInt8Vector(), utf8Bytes());
        REQUIRE_EQUAL(
            StringEncoder{text}.encode(StringEncoding::Utf8, StringBomMode::Require).toUInt8Vector(),
            withBom({0xEFU, 0xBBU, 0xBFU}, utf8Bytes()));
    }

    void testEncodeUtf16AndUtf32Endianness() {
        const auto text = sampleText();

        REQUIRE_EQUAL(
            StringEncoder{text}.encode(StringEncoding::Utf16).toUInt8Vector(), withBom({0xFFU, 0xFEU}, utf16Le()));
        REQUIRE_EQUAL(
            StringEncoder{text}.encode(StringEncoding::Utf16LittleEndian, StringBomMode::Reject).toUInt8Vector(),
            utf16Le());
        REQUIRE_EQUAL(
            StringEncoder{text}.encode(StringEncoding::Utf16BigEndian, StringBomMode::Reject).toUInt8Vector(),
            utf16Be());

        REQUIRE_EQUAL(
            StringEncoder{text}.encode(StringEncoding::Utf32).toUInt8Vector(),
            withBom({0xFFU, 0xFEU, 0x00U, 0x00U}, utf32Le()));
        REQUIRE_EQUAL(
            StringEncoder{text}.encode(StringEncoding::Utf32LittleEndian, StringBomMode::Reject).toUInt8Vector(),
            utf32Le());
        REQUIRE_EQUAL(
            StringEncoder{text}.encode(StringEncoding::Utf32BigEndian, StringBomMode::Reject).toUInt8Vector(),
            utf32Be());
    }

    void testDecodeRoundTripsAllEncodings() {
        requireDecode(StringEncoding::Utf8, makeBlock(utf8Bytes()));
        requireDecode(StringEncoding::Utf16LittleEndian, makeBlock(utf16Le()));
        requireDecode(StringEncoding::Utf16BigEndian, makeBlock(utf16Be()));
        requireDecode(StringEncoding::Utf32LittleEndian, makeBlock(utf32Le()));
        requireDecode(StringEncoding::Utf32BigEndian, makeBlock(utf32Be()));
    }

    void testDecodeBomModes() {
        REQUIRE_EQUAL(
            StringConverter{
                StringDecoder{makeBlock(withBom({0xEFU, 0xBBU, 0xBFU}, utf8Bytes()))}.toU8String(StringEncoding::Utf8)}
                .toStdU32String(),
            sampleU32());
        REQUIRE_EQUAL(
            StringConverter{
                StringDecoder{makeBlock(withBom({0xFEU, 0xFFU}, utf16Be()))}.toU8String(StringEncoding::Utf16)}
                .toStdU32String(),
            sampleU32());
        REQUIRE_EQUAL(
            StringConverter{StringDecoder{makeBlock(withBom({0x00U, 0x00U, 0xFEU, 0xFFU}, utf32Be()))}.toU8String(
                                StringEncoding::Utf32)}
                .toStdU32String(),
            sampleU32());

        REQUIRE_THROWS(StringDecoder{makeBlock(utf16Le())}.toU8String(StringEncoding::Utf16, StringBomMode::Require));
        REQUIRE_THROWS(
            StringDecoder{makeBlock(withBom({0xFFU, 0xFEU}, utf16Le()))}.toU8String(
                StringEncoding::Utf16LittleEndian, StringBomMode::Reject));
        REQUIRE_THROWS(
            StringDecoder{makeBlock(withBom({0xFEU, 0xFFU}, utf16Be()))}.toU8String(StringEncoding::Utf16LittleEndian));
        REQUIRE_THROWS(
            StringDecoder{makeBlock(withBom({0x00U, 0x00U, 0xFEU, 0xFFU}, utf32Be()))}.toU8String(
                StringEncoding::Utf32LittleEndian));
    }

    void testDecodeErrorModes() {
        const auto malformedUtf8 = makeBlock({0x41U, 0xC0U, 0x42U});
        REQUIRE_EQUAL(
            StringConverter{StringDecoder{malformedUtf8}.toU8String(StringEncoding::Utf8)}.toStdU32String(),
            std::u32string{U"A\uFFFDB"});
        REQUIRE_EQUAL(
            StringConverter{StringDecoder{malformedUtf8}.toU8String(
                                StringEncoding::Utf8, StringBomMode::Automatic, EncodingErrorMode::Ignore)}
                .toStdU32String(),
            std::u32string{U"AB"});
        REQUIRE_THROWS(
            StringDecoder{malformedUtf8}.toU8String(
                StringEncoding::Utf8, StringBomMode::Automatic, EncodingErrorMode::Throw));

        const auto malformedUtf16 = makeBlock({0x00U, 0xD8U, 0x41U, 0x00U});
        REQUIRE_EQUAL(
            StringConverter{StringDecoder{malformedUtf16}.toU8String(StringEncoding::Utf16LittleEndian)}
                .toStdU32String(),
            std::u32string{U"\uFFFDA"});
        REQUIRE_EQUAL(
            StringConverter{StringDecoder{malformedUtf16}.toU8String(
                                StringEncoding::Utf16LittleEndian, StringBomMode::Automatic, EncodingErrorMode::Ignore)}
                .toStdU32String(),
            std::u32string{U"A"});
        REQUIRE_THROWS(
            StringDecoder{malformedUtf16}.toU8String(
                StringEncoding::Utf16LittleEndian, StringBomMode::Automatic, EncodingErrorMode::Throw));

        const auto malformedUtf32 = makeBlock({0x00U, 0xD8U, 0x00U, 0x00U, 0x41U, 0x00U, 0x00U, 0x00U});
        REQUIRE_EQUAL(
            StringConverter{StringDecoder{malformedUtf32}.toU8String(StringEncoding::Utf32LittleEndian)}
                .toStdU32String(),
            std::u32string{U"\uFFFDA"});
        REQUIRE_EQUAL(
            StringConverter{StringDecoder{malformedUtf32}.toU8String(
                                StringEncoding::Utf32LittleEndian, StringBomMode::Automatic, EncodingErrorMode::Ignore)}
                .toStdU32String(),
            std::u32string{U"A"});
        REQUIRE_THROWS(
            StringDecoder{malformedUtf32}.toU8String(
                StringEncoding::Utf32LittleEndian, StringBomMode::Automatic, EncodingErrorMode::Throw));
    }

    void testDecodeTruncatedUtf16AndUtf32Tails() {
        REQUIRE_EQUAL(
            StringConverter{
                StringDecoder{makeBlock({0x41U, 0x00U, 0x99U})}.toU8String(StringEncoding::Utf16LittleEndian)}
                .toStdU32String(),
            std::u32string{U"A\uFFFD"});
        REQUIRE_EQUAL(
            StringConverter{StringDecoder{makeBlock({0x41U, 0x00U, 0x99U})}.toU8String(
                                StringEncoding::Utf16LittleEndian, StringBomMode::Automatic, EncodingErrorMode::Ignore)}
                .toStdU32String(),
            std::u32string{U"A"});
        REQUIRE_THROWS(
            StringDecoder{makeBlock({0x41U, 0x00U, 0x99U})}.toU8String(
                StringEncoding::Utf16LittleEndian, StringBomMode::Automatic, EncodingErrorMode::Throw));

        REQUIRE_EQUAL(
            StringConverter{StringDecoder{makeBlock({0x41U, 0x00U, 0x00U, 0x00U, 0x99U})}.toU8String(
                                StringEncoding::Utf32LittleEndian)}
                .toStdU32String(),
            std::u32string{U"A\uFFFD"});
        REQUIRE_EQUAL(
            StringConverter{StringDecoder{makeBlock({0x41U, 0x00U, 0x00U, 0x00U, 0x99U})}.toU8String(
                                StringEncoding::Utf32LittleEndian, StringBomMode::Automatic, EncodingErrorMode::Ignore)}
                .toStdU32String(),
            std::u32string{U"A"});
        REQUIRE_THROWS(
            StringDecoder{makeBlock({0x41U, 0x00U, 0x00U, 0x00U, 0x99U})}.toU8String(
                StringEncoding::Utf32LittleEndian, StringBomMode::Automatic, EncodingErrorMode::Throw));
    }

    void testUtf16AndUtf32ReaderDecoding() {
        auto utf16Text = std::u32string{};
        const auto utf16Data = makeBlock({0x78U, 0x00U, 0x41U, 0x00U, 0x3DU, 0xD8U, 0x00U, 0xDEU});
        auto utf16Reader = ByteReader{utf16Data};
        utf16Reader.setPosition(ByteIndex{2U});
        REQUIRE(
            el::text::impl::utf16::forEachDecodedCharacter<EncodingErrorMode::Throw>(
                utf16Reader, [&](const Char character) -> void { utf16Text.push_back(character.toRawValue()); }));
        REQUIRE_EQUAL(utf16Text, std::u32string{U"A😀"});
        REQUIRE_EQUAL(utf16Reader.position(), ByteIndex{8U});

        auto invalidUtf16Reader = ByteReader{makeBlock({0x00U, 0xD8U, 0x41U, 0x00U})};
        REQUIRE_THROWS(
            el::text::impl::utf16::forEachDecodedCharacter<EncodingErrorMode::Throw>(
                invalidUtf16Reader, [](Char) -> void {}));
        REQUIRE_EQUAL(invalidUtf16Reader.position(), ByteIndex::zero());

        auto utf32Text = std::u32string{};
        const auto utf32Data = makeBlock({0x78U, 0x00U, 0x00U, 0x00U, 0x41U, 0x00U, 0x00U, 0x00U});
        auto utf32Reader = ByteReader{utf32Data};
        utf32Reader.setPosition(ByteIndex{4U});
        REQUIRE(
            el::text::impl::utf32::forEachValidatedCharacter<EncodingErrorMode::Throw>(
                utf32Reader, [&](const Char character) -> void { utf32Text.push_back(character.toRawValue()); }));
        REQUIRE_EQUAL(utf32Text, std::u32string{U"A"});
        REQUIRE_EQUAL(utf32Reader.position(), ByteIndex{8U});

        auto invalidUtf32Reader = ByteReader{makeBlock({0x00U, 0xD8U, 0x00U, 0x00U})};
        REQUIRE_THROWS(
            el::text::impl::utf32::forEachValidatedCharacter<EncodingErrorMode::Throw>(
                invalidUtf32Reader, [](Char) -> void {}));
        REQUIRE_EQUAL(invalidUtf32Reader.position(), ByteIndex::zero());
    }

    void testEncodeEntryPointsAndSlices() {
        const auto source = U8String{std::u8string_view{u8"xxA¢€😀yy"}};
        const auto view = U8StringView{source}.slice(ByteRange{ByteIndex{2U}, ByteLength{10U}});
        const auto charView = source.toCharView().slice(CpRange{CpIndex{2U}, CpLength{4U}}).toCharView();

        REQUIRE_EQUAL(
            StringEncoder{source}.encode(StringEncoding::Utf8, StringBomMode::Reject).toUInt8Vector(), withAffixes());
        REQUIRE_EQUAL(
            StringEncoder{view}.encode(StringEncoding::Utf8, StringBomMode::Reject).toUInt8Vector(), utf8Bytes());
        REQUIRE_EQUAL(
            StringEncoder{charView}.encode(StringEncoding::Utf16LittleEndian, StringBomMode::Reject).toUInt8Vector(),
            utf16Le());
    }

    void testHelperMethods() {
        using Tools = el::text::impl::U8StringEncodingTools;

        REQUIRE(el::text::impl::utf8::hasBom(makeBlock({0xEFU, 0xBBU, 0xBFU, 0x41U})));
        REQUIRE(el::text::impl::utf16::hasLittleEndianBom(makeBlock({0xFFU, 0xFEU, 0x41U, 0x00U})));
        REQUIRE(el::text::impl::utf16::hasBigEndianBom(makeBlock({0xFEU, 0xFFU, 0x00U, 0x41U})));
        REQUIRE(el::text::impl::utf32::hasLittleEndianBom(makeBlock({0xFFU, 0xFEU, 0x00U, 0x00U})));
        REQUIRE(el::text::impl::utf32::hasBigEndianBom(makeBlock({0x00U, 0x00U, 0xFEU, 0xFFU})));
        REQUIRE_EQUAL(el::text::impl::utf8::bomLength(), std::size_t{3U});
        REQUIRE_EQUAL(el::text::impl::utf16::bomLength(), std::size_t{2U});
        REQUIRE_EQUAL(el::text::impl::utf32::bomLength(), std::size_t{4U});
        REQUIRE(el::text::impl::utf16::isEncoding(StringEncoding::Utf16BigEndian));
        REQUIRE_FALSE(el::text::impl::utf16::isEncoding(StringEncoding::Utf32BigEndian));
        REQUIRE(el::text::impl::utf32::isEncoding(StringEncoding::Utf32LittleEndian));
        REQUIRE_EQUAL(Tools::defaultEndianness(StringEncoding::Utf16BigEndian), Endianness::Big);
        REQUIRE(Tools::shouldWriteBom(StringEncoding::Utf16, StringBomMode::Automatic));
        REQUIRE_FALSE(Tools::shouldWriteBom(StringEncoding::Utf8, StringBomMode::Automatic));

        const auto utf16Data = makeBlock({0x34U, 0x12U, 0x12U, 0x34U});
        auto utf16Reader = ByteReader{utf16Data};
        REQUIRE_EQUAL(el::text::impl::utf16::readCodeUnit(utf16Reader), char16_t{0x1234U});
        utf16Reader.setEndianness(Endianness::Big);
        REQUIRE_EQUAL(el::text::impl::utf16::readCodeUnit(utf16Reader), char16_t{0x1234U});

        const auto utf32Data = makeBlock({0x78U, 0x56U, 0x34U, 0x12U, 0x12U, 0x34U, 0x56U, 0x78U});
        auto utf32Reader = ByteReader{utf32Data};
        REQUIRE_EQUAL(el::text::impl::utf32::readCodeUnit(utf32Reader), char32_t{0x12345678U});
        utf32Reader.setEndianness(Endianness::Big);
        REQUIRE_EQUAL(el::text::impl::utf32::readCodeUnit(utf32Reader), char32_t{0x12345678U});

        const auto layout = Tools::resolveBomLayout(
            makeBlock({0xFEU, 0xFFU, 0x00U, 0x41U}), StringEncoding::Utf16, StringBomMode::Automatic);
        REQUIRE_EQUAL(layout.endianness, Endianness::Big);
        REQUIRE_EQUAL(layout.start, std::size_t{2U});
    }

private:
    [[nodiscard]] static auto sampleText() -> U8String { return U8String{std::u8string_view{u8"A¢€😀"}}; }

    [[nodiscard]] static auto sampleU32() -> std::u32string { return std::u32string{U"A¢€😀"}; }

    [[nodiscard]] static auto makeBlock(std::initializer_list<uint8_t> bytes) -> ByteBlock {
        return ByteBlock{std::vector<uint8_t>{bytes}};
    }

    [[nodiscard]] static auto makeBlock(const std::vector<uint8_t> &bytes) -> ByteBlock { return ByteBlock{bytes}; }

    [[nodiscard]] static auto withBom(std::initializer_list<uint8_t> bom, const std::vector<uint8_t> &bytes)
        -> std::vector<uint8_t> {
        auto result = std::vector<uint8_t>{bom};
        result.insert(result.end(), bytes.begin(), bytes.end());
        return result;
    }

    [[nodiscard]] static auto withAffixes() -> std::vector<uint8_t> {
        auto result = std::vector<uint8_t>{0x78U, 0x78U};
        const auto bytes = utf8Bytes();
        result.insert(result.end(), bytes.begin(), bytes.end());
        result.push_back(0x79U);
        result.push_back(0x79U);
        return result;
    }

    void requireDecode(const StringEncoding encoding, const ByteBlock &data) {
        REQUIRE_EQUAL(StringConverter{StringDecoder{data}.toU8String(encoding)}.toStdU32String(), sampleU32());
    }

    [[nodiscard]] static auto utf8Bytes() -> std::vector<uint8_t> {
        return {0x41U, 0xC2U, 0xA2U, 0xE2U, 0x82U, 0xACU, 0xF0U, 0x9FU, 0x98U, 0x80U};
    }

    [[nodiscard]] static auto utf16Le() -> std::vector<uint8_t> {
        return {0x41U, 0x00U, 0xA2U, 0x00U, 0xACU, 0x20U, 0x3DU, 0xD8U, 0x00U, 0xDEU};
    }

    [[nodiscard]] static auto utf16Be() -> std::vector<uint8_t> {
        return {0x00U, 0x41U, 0x00U, 0xA2U, 0x20U, 0xACU, 0xD8U, 0x3DU, 0xDEU, 0x00U};
    }

    [[nodiscard]] static auto utf32Le() -> std::vector<uint8_t> {
        return {
            0x41U,
            0x00U,
            0x00U,
            0x00U,
            0xA2U,
            0x00U,
            0x00U,
            0x00U,
            0xACU,
            0x20U,
            0x00U,
            0x00U,
            0x00U,
            0xF6U,
            0x01U,
            0x00U};
    }

    [[nodiscard]] static auto utf32Be() -> std::vector<uint8_t> {
        return {
            0x00U,
            0x00U,
            0x00U,
            0x41U,
            0x00U,
            0x00U,
            0x00U,
            0xA2U,
            0x00U,
            0x00U,
            0x20U,
            0xACU,
            0x00U,
            0x01U,
            0xF6U,
            0x00U};
    }
};
