// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/EncodingError.hpp>
#include <erbsland/text/impl/UnsafeDecodeBufferAccess.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringDecodeBuffer.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <algorithm>
#include <string>
#include <vector>

using el::mem::Byte;
using el::text::Char;
using el::text::EncodingError;
using el::text::EncodingErrorMode;
using el::text::StringBomMode;
using el::text::StringConverter;
using el::text::StringDecodeBuffer;
using el::text::StringEncoding;
using el::unit::ByteLength;
using el::unit::CpLength;

namespace th = erbsland::unittest::th;

TESTED_TARGETS(StringDecodeBuffer UnsafeDecodeBufferAccess)
class StringDecodeBufferTest final : public el::UnitTest {
public:
    [[nodiscard]] static auto toStdString(const el::text::String &text) -> std::string {
        return StringConverter{text}.toStdString();
    }

public:
    void testUtf8SplitCodePoint() {
        auto buffer = StringDecodeBuffer{ByteLength{8U}, StringEncoding::Utf8};

        buffer.write(std::string_view{"A"});
        REQUIRE_EQUAL(buffer.decodableCharacters(), CpLength{1U});
        REQUIRE_EQUAL(toStdString(buffer.takeString()), std::string{"A"});

        buffer.write(std::vector<uint8_t>{0xC3U});
        REQUIRE_EQUAL(buffer.codePointStatus(), StringDecodeBuffer::CodePointStatus::NeedMoreData);
        REQUIRE(buffer.takeString().isEmpty());

        buffer.write(std::vector<uint8_t>{0xA4U});
        REQUIRE_EQUAL(buffer.codePointStatus(), StringDecodeBuffer::CodePointStatus::Complete);
        REQUIRE_EQUAL(toStdString(buffer.takeString()), th::stdStringFromHex("C3 A4"));
        REQUIRE(buffer.isEmpty());
    }

    void testUtf8BoundaryStatus() {
        auto buffer = StringDecodeBuffer{ByteLength{8U}, StringEncoding::Utf8};

        buffer.write(std::string_view{"A"});
        REQUIRE_EQUAL(buffer.codePointStatus(), StringDecodeBuffer::CodePointStatus::Complete);
        buffer.write(std::vector<uint8_t>{0xC3U});
        REQUIRE_EQUAL(buffer.codePointStatus(), StringDecodeBuffer::CodePointStatus::NeedMoreData);
        buffer.write(std::vector<uint8_t>{0xA4U});
        REQUIRE_EQUAL(buffer.codePointStatus(), StringDecodeBuffer::CodePointStatus::Complete);

        buffer.reset();
        buffer.write(std::vector<uint8_t>{0xE0U, 0x80U});
        REQUIRE_EQUAL(buffer.codePointStatus(), StringDecodeBuffer::CodePointStatus::Invalid);
    }

    void testUtf16SplitSurrogatePair() {
        auto buffer = StringDecodeBuffer{ByteLength{8U}, StringEncoding::Utf16LittleEndian};

        buffer.write(std::vector<uint8_t>{0x3DU, 0xD8U});
        REQUIRE_EQUAL(buffer.codePointStatus(), StringDecodeBuffer::CodePointStatus::NeedMoreData);
        REQUIRE(buffer.takeString().isEmpty());

        buffer.write(std::vector<uint8_t>{0x00U, 0xDEU});
        REQUIRE_EQUAL(buffer.decodableCharacters(), CpLength{1U});
        REQUIRE_EQUAL(toStdString(buffer.takeString()), th::stdStringFromHex("F0 9F 98 80"));
    }

    void testUtf32SplitCodePoint() {
        auto buffer = StringDecodeBuffer{ByteLength{8U}, StringEncoding::Utf32BigEndian};

        buffer.write(std::vector<uint8_t>{0x00U, 0x01U});
        REQUIRE_EQUAL(buffer.codePointStatus(), StringDecodeBuffer::CodePointStatus::NeedMoreData);
        buffer.write(std::vector<uint8_t>{0xF6U, 0x00U});

        REQUIRE_EQUAL(buffer.decodableCharacters(), CpLength{1U});
        REQUIRE_EQUAL(toStdString(buffer.takeString()), th::stdStringFromHex("F0 9F 98 80"));
    }

    void testBomSplitAcrossChunks() {
        auto buffer = StringDecodeBuffer{ByteLength{8U}, StringEncoding::Utf16};

        buffer.write(std::vector<uint8_t>{0xFFU});
        REQUIRE_EQUAL(buffer.codePointStatus(), StringDecodeBuffer::CodePointStatus::NeedMoreData);
        buffer.write(std::vector<uint8_t>{0xFEU});
        REQUIRE(buffer.takeString().isEmpty());
        buffer.write(std::vector<uint8_t>{0x41U, 0x00U});

        REQUIRE_EQUAL(buffer.effectiveEncoding(), StringEncoding::Utf16LittleEndian);
        REQUIRE_EQUAL(toStdString(buffer.takeString()), std::string{"A"});
    }

    void testFinalIncompleteSequenceErrorModes() {
        auto replaceBuffer = StringDecodeBuffer{
            ByteLength{4U}, StringEncoding::Utf8, StringBomMode::Automatic, EncodingErrorMode::Replace};
        replaceBuffer.write(std::vector<uint8_t>{0xC3U});
        REQUIRE(replaceBuffer.takeString().isEmpty());
        replaceBuffer.finish();
        REQUIRE_EQUAL(toStdString(replaceBuffer.takeString()), th::stdStringFromHex("EF BF BD"));

        auto ignoreBuffer = StringDecodeBuffer{
            ByteLength{4U}, StringEncoding::Utf8, StringBomMode::Automatic, EncodingErrorMode::Ignore};
        ignoreBuffer.write(std::vector<uint8_t>{0xC3U});
        ignoreBuffer.finish();
        REQUIRE(ignoreBuffer.takeString().isEmpty());
        REQUIRE(ignoreBuffer.isEmpty());

        auto throwBuffer = StringDecodeBuffer{
            ByteLength{4U}, StringEncoding::Utf8, StringBomMode::Automatic, EncodingErrorMode::Throw};
        throwBuffer.write(std::vector<uint8_t>{0xC3U});
        throwBuffer.finish();
        REQUIRE_THROWS_AS(EncodingError, throwBuffer.takeString());
    }

    void testPeekTakeAndRingWrap() {
        auto buffer = StringDecodeBuffer{ByteLength{6U}, StringEncoding::Utf8};

        buffer.write(std::string_view{"abcd"});
        REQUIRE_EQUAL(toStdString(buffer.takeString(CpLength{2U})), std::string{"ab"});
        buffer.write(std::string_view{"efg"});

        REQUIRE_EQUAL(toStdString(buffer.peekString()), std::string{"cdefg"});
        REQUIRE_EQUAL(toStdString(buffer.peekString()), std::string{"cdefg"});
        REQUIRE_EQUAL(toStdString(buffer.takeString(CpLength{3U})), std::string{"cde"});
        REQUIRE_EQUAL(toStdString(buffer.takeString()), std::string{"fg"});
        REQUIRE(buffer.isEmpty());
    }

    void testReadChar() {
        auto utf8Buffer = StringDecodeBuffer{ByteLength{8U}, StringEncoding::Utf8};
        utf8Buffer.write(std::string_view{"A"});
        REQUIRE_EQUAL(utf8Buffer.readChar().value(), Char{U'A'});
        REQUIRE_FALSE(utf8Buffer.readChar().has_value());

        utf8Buffer.write(std::vector<uint8_t>{0xC3U});
        REQUIRE_FALSE(utf8Buffer.readChar().has_value());
        utf8Buffer.write(std::vector<uint8_t>{0xA4U});
        REQUIRE_EQUAL(utf8Buffer.readChar().value(), Char{0x00E4U});

        auto utf16Buffer = StringDecodeBuffer{ByteLength{8U}, StringEncoding::Utf16LittleEndian};
        utf16Buffer.write(std::vector<uint8_t>{0x3DU, 0xD8U, 0x00U, 0xDEU});
        REQUIRE_EQUAL(utf16Buffer.readChar().value(), Char{0x1F600U});

        auto utf32Buffer = StringDecodeBuffer{ByteLength{8U}, StringEncoding::Utf32BigEndian};
        utf32Buffer.write(std::vector<uint8_t>{0x00U, 0x01U, 0xF6U, 0x00U});
        REQUIRE_EQUAL(utf32Buffer.readChar().value(), Char{0x1F600U});
    }

    void testReadCharBomAndErrorModes() {
        auto bomBuffer = StringDecodeBuffer{ByteLength{8U}, StringEncoding::Utf16};
        bomBuffer.write(std::vector<uint8_t>{0xFFU, 0xFEU, 0x41U, 0x00U});
        REQUIRE_EQUAL(bomBuffer.readChar().value(), Char{U'A'});
        REQUIRE_EQUAL(bomBuffer.effectiveEncoding(), StringEncoding::Utf16LittleEndian);

        auto replaceBuffer = StringDecodeBuffer{
            ByteLength{4U}, StringEncoding::Utf8, StringBomMode::Automatic, EncodingErrorMode::Replace};
        replaceBuffer.write(std::vector<uint8_t>{0xC3U});
        replaceBuffer.finish();
        REQUIRE_EQUAL(replaceBuffer.readChar().value(), Char::replacement());

        auto ignoreBuffer = StringDecodeBuffer{
            ByteLength{4U}, StringEncoding::Utf8, StringBomMode::Automatic, EncodingErrorMode::Ignore};
        ignoreBuffer.write(std::vector<uint8_t>{0xFFU, 0x41U});
        REQUIRE_EQUAL(ignoreBuffer.readChar().value(), Char{U'A'});
        REQUIRE(ignoreBuffer.isEmpty());

        auto throwBuffer = StringDecodeBuffer{
            ByteLength{4U}, StringEncoding::Utf8, StringBomMode::Automatic, EncodingErrorMode::Throw};
        throwBuffer.write(std::vector<uint8_t>{0xFFU});
        REQUIRE_THROWS_AS(EncodingError, throwBuffer.readChar());
        REQUIRE_EQUAL(throwBuffer.byteLength(), ByteLength{1U});
    }

    void testTakeStringLine() {
        auto buffer = StringDecodeBuffer{ByteLength{16U}, StringEncoding::Utf8};
        buffer.write(std::string_view{"ab\ncd"});
        REQUIRE_EQUAL(toStdString(buffer.takeStringLine()), std::string{"ab\n"});
        REQUIRE_EQUAL(toStdString(buffer.takeStringLine()), std::string{"cd"});
        REQUIRE(buffer.isEmpty());

        buffer.write(std::string_view{"abcd\n"});
        REQUIRE_EQUAL(toStdString(buffer.takeStringLine(CpLength{2U})), std::string{"ab"});
        REQUIRE_EQUAL(toStdString(buffer.takeStringLine()), std::string{"cd\n"});

        buffer.write(std::string_view{"a\rb\n"});
        REQUIRE_EQUAL(toStdString(buffer.takeStringLine()), std::string{"a\rb\n"});
    }

    void testTakeStringLineSplitAndWideEncodings() {
        auto utf8Buffer = StringDecodeBuffer{ByteLength{16U}, StringEncoding::Utf8};
        utf8Buffer.write(std::vector<uint8_t>{0xF0U, 0x9FU});
        REQUIRE(utf8Buffer.takeStringLine().isEmpty());
        utf8Buffer.write(std::vector<uint8_t>{0x98U, 0x80U, 0x0AU});
        REQUIRE_EQUAL(
            toStdString(utf8Buffer.takeStringLine()), th::stdStringFromHex("F0 9F 98 80") + std::string{"\n"});

        auto utf16Buffer = StringDecodeBuffer{ByteLength{16U}, StringEncoding::Utf16LittleEndian};
        utf16Buffer.write(std::vector<uint8_t>{0x41U, 0x00U, 0x0AU, 0x00U, 0x42U, 0x00U});
        REQUIRE_EQUAL(toStdString(utf16Buffer.takeStringLine()), std::string{"A\n"});
        REQUIRE_EQUAL(toStdString(utf16Buffer.takeStringLine()), std::string{"B"});

        auto utf32Buffer = StringDecodeBuffer{ByteLength{16U}, StringEncoding::Utf32BigEndian};
        utf32Buffer.write(
            std::vector<uint8_t>{0x00U, 0x00U, 0x00U, 0x41U, 0x00U, 0x00U, 0x00U, 0x0AU, 0x00U, 0x00U, 0x00U, 0x42U});
        REQUIRE_EQUAL(toStdString(utf32Buffer.takeStringLine()), std::string{"A\n"});
        REQUIRE_EQUAL(toStdString(utf32Buffer.takeStringLine()), std::string{"B"});
    }

    void testUnsafeAccess() {
        auto buffer = StringDecodeBuffer{ByteLength{8U}, StringEncoding::Utf8};
        auto access = el::text::impl::UnsafeDecodeBufferAccess{buffer};
        auto span = access.writableSpan();
        const auto text = std::string_view{"abc"};
        for (auto i = std::size_t{0}; i < text.size(); ++i) {
            span[i] = Byte{static_cast<uint8_t>(text[i])};
        }
        access.commitWritten(ByteLength{3U});

        REQUIRE_EQUAL(toStdString(buffer.takeString()), std::string{"abc"});
        REQUIRE_EQUAL(access.consumedByteLength(), ByteLength{3U});

        buffer.reset();
        REQUIRE_EQUAL(access.consumedByteLength(), ByteLength::zero());
    }
};
