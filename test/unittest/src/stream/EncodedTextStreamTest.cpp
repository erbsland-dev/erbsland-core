// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/EncodingError.hpp>
#include <erbsland/err/StreamError.hpp>
#include <erbsland/stream/impl/EncodedTextInputStream.hpp>
#include <erbsland/stream/impl/EncodedTextOutputStream.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringEncoder.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

using el::err::EncodingError;
using el::err::StreamError;
using el::mem::Byte;
using el::mem::ByteBlock;
using el::stream::ByteInputStream;
using el::stream::ByteOutputStream;
using el::stream::TextInputStream;
using el::unit::ByteLength;
using el::unit::CpLength;
using namespace el::text;

TESTED_TARGETS(EncodedTextInputStream EncodedTextOutputStream)
class EncodedTextStreamTest final : public el::UnitTest {
    class MemoryInputStream final : public ByteInputStream {
    public:
        explicit MemoryInputStream(const ByteBlock &bytes) : _bytes{bytes.toByteVector()} {}

    public: // implement ByteInputStream
        [[nodiscard]] auto isOpen() const noexcept -> bool override { return _open; }

        void close() override { _open = false; }

        [[nodiscard]] auto read(std::span<Byte> destination) -> ByteLength override {
            if (!_open) {
                throw StreamError{"Closed."};
            }
            const auto available = _bytes.size() - _position;
            const auto count = std::min(destination.size(), available);
            std::copy_n(_bytes.data() + _position, count, destination.data());
            _position += count;
            return ByteLength::fromSizeT(count);
        }

    public:
        using ByteInputStream::read;

    private:
        std::vector<Byte> _bytes;
        std::size_t _position{0};
        bool _open{true};
    };

    class MemoryOutputStream final : public ByteOutputStream {
    public: // implement ByteOutputStream
        [[nodiscard]] auto isOpen() const noexcept -> bool override { return _open; }

        void flush() override {}

        void close() override { _open = false; }

        void write(std::span<const Byte> bytes) override {
            if (!_open) {
                throw StreamError{"Closed."};
            }
            for (const auto byte : bytes) {
                data.push_back(byte.toUInt8());
            }
        }

    public:
        using ByteOutputStream::write;

    public:
        std::vector<uint8_t> data;

    private:
        bool _open{true};
    };

public:
    void testReadCharAndRead() {
        auto bytes = StringEncoder{String{std::string_view{"Hello"}}}.encode(StringEncoding::Utf8);
        auto byteStream = std::make_shared<MemoryInputStream>(bytes);
        auto stream = el::stream::impl::EncodedTextInputStream{byteStream, StringEncoding::Utf8};
        auto &textStream = static_cast<TextInputStream &>(stream);

        REQUIRE_EQUAL(textStream.encoding(), StringEncoding::Utf8);
        REQUIRE_EQUAL(textStream.effectiveEncoding(), StringEncoding::Utf8);
        REQUIRE_EQUAL(textStream.readChar().value(), Char{U'H'});
        REQUIRE_EQUAL(StringConverter{textStream.read(CpLength{2U}).value()}.toStdString(), std::string{"el"});
        REQUIRE_EQUAL(StringConverter{textStream.read().value()}.toStdString(), std::string{"lo"});
        REQUIRE_FALSE(textStream.read().has_value());
    }

    void testReadLinesKeepEndingsAndTruncate() {
        auto bytes = StringEncoder{String{std::string_view{"a\r\nb\rc\nlast"}}}.encode(StringEncoding::Utf8);
        auto byteStream = std::make_shared<MemoryInputStream>(bytes);
        auto stream = el::stream::impl::EncodedTextInputStream{byteStream, StringEncoding::Utf8};
        auto &textStream = static_cast<TextInputStream &>(stream);

        REQUIRE_EQUAL(StringConverter{textStream.readLine(CpLength{2U}).value()}.toStdString(), std::string{"a\r"});
        REQUIRE_EQUAL(StringConverter{textStream.readLine(CpLength{8U}).value()}.toStdString(), std::string{"\n"});
        REQUIRE_EQUAL(StringConverter{textStream.readLine(CpLength{8U}).value()}.toStdString(), std::string{"b\r"});
        REQUIRE_EQUAL(StringConverter{textStream.readLine(CpLength{8U}).value()}.toStdString(), std::string{"c\n"});
        REQUIRE_EQUAL(StringConverter{textStream.readAll(CpLength{2U})}.toStdString(), std::string{"la"});
        REQUIRE_EQUAL(StringConverter{textStream.readAll()}.toStdString(), std::string{"st"});
        REQUIRE_FALSE(textStream.readLine().has_value());
    }

    void testZeroMaximumReturnsEmptyWithoutAdvancing() {
        auto bytes = StringEncoder{String{std::string_view{"abc"}}}.encode(StringEncoding::Utf8);
        auto byteStream = std::make_shared<MemoryInputStream>(bytes);
        auto stream = el::stream::impl::EncodedTextInputStream{byteStream, StringEncoding::Utf8};

        REQUIRE(stream.read(CpLength::zero()).value().isEmpty());
        REQUIRE_EQUAL(StringConverter{stream.readAll(CpLength{3U})}.toStdString(), std::string{"abc"});
    }

    void testBomAndEffectiveEncoding() {
        auto bytes = StringEncoder{String{std::string_view{"A"}}}.encode(StringEncoding::Utf16);
        auto byteStream = std::make_shared<MemoryInputStream>(bytes);
        auto stream = el::stream::impl::EncodedTextInputStream{byteStream, StringEncoding::Utf16};

        REQUIRE_EQUAL(stream.effectiveEncoding(), StringEncoding::Utf16LittleEndian);
        REQUIRE_EQUAL(StringConverter{stream.readAll()}.toStdString(), std::string{"A"});
        REQUIRE_EQUAL(stream.effectiveEncoding(), StringEncoding::Utf16LittleEndian);
    }

    void testThrowModePropagatesEncodingErrors() {
        auto bytes = ByteBlock{std::vector<uint8_t>{0xffU}};
        auto byteStream = std::make_shared<MemoryInputStream>(bytes);
        auto stream = el::stream::impl::EncodedTextInputStream{
            byteStream, StringEncoding::Utf8, StringBomMode::Automatic, EncodingErrorMode::Throw};

        REQUIRE_THROWS_AS(EncodingError, stream.readAll());
    }

    void testOutputUtf8AndBomOnlyOnce() {
        using namespace el::text::literals;

        const auto byteStream = std::make_shared<MemoryOutputStream>();
        auto stream = el::stream::impl::EncodedTextOutputStream{byteStream, StringEncoding::Utf16};

        stream.print("A");
        stream.writeLine("B"_el);

        REQUIRE_EQUAL(byteStream->data, std::vector<uint8_t>({0xffU, 0xfeU, 0x41U, 0x00U, 0x42U, 0x00U, 0x0aU, 0x00U}));
        REQUIRE_EQUAL(stream.encoding(), StringEncoding::Utf16);
        REQUIRE_EQUAL(stream.effectiveEncoding(), StringEncoding::Utf16LittleEndian);
    }
};
