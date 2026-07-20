// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/ParameterError.hpp>
#include <erbsland/stream/impl/EncodedTextInputStream.hpp>
#include <erbsland/stream/impl/EncodedTextOutputStream.hpp>
#include <erbsland/stream/StreamError.hpp>
#include <erbsland/text/EncodingError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringEncoder.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <algorithm>
#include <chrono>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace el::text::literals;

using el::mem::Byte;
using el::mem::ByteBlock;
using el::stream::ByteInputStream;
using el::stream::ByteOutputStream;
using el::stream::StreamError;
using el::stream::StreamReadStatus;
using el::stream::TextInputStream;
using el::text::EncodingError;
using el::unit::ByteLength;
using el::unit::CpLength;
using namespace el::text;
using namespace el::text::literals;

namespace th = erbsland::unittest::th;

TESTED_TARGETS(EncodedTextInputStream EncodedTextOutputStream)
class EncodedTextStreamTest final : public el::UnitTest {
    class MemoryInputStream final : public ByteInputStream {
    public:
        explicit MemoryInputStream(
            const ByteBlock &bytes,
            const std::size_t maximumRead = std::numeric_limits<std::size_t>::max(),
            const std::optional<std::size_t> timeoutCall = std::nullopt,
            const bool readyAtEnd = true) :
            _bytes{bytes.toByteVector()},
            _maximumRead{maximumRead},
            _timeoutCall{timeoutCall},
            _readyAtEnd{readyAtEnd} {}

    public: // implement ByteInputStream
        [[nodiscard]] auto inputSettings() const noexcept -> const el::stream::InputStreamSettings & override {
            return _settings;
        }
        [[nodiscard]] auto state() const noexcept -> el::stream::StreamState override { return _state; }
        [[nodiscard]] auto isReady() const noexcept -> bool override {
            return _readyAtEnd || _position < _bytes.size();
        }
        [[nodiscard]] auto waitForReady() -> el::stream::StreamWaitStatus override {
            return el::stream::StreamWaitStatus::Ready;
        }
        auto close() -> el::stream::StreamCloseStatus override {
            _state = el::stream::StreamState::Closed;
            return el::stream::StreamCloseStatus::Closed;
        }
        void abort() noexcept override { _state = el::stream::StreamState::Closed; }

    protected:
        [[nodiscard]] auto readFromSource(std::span<Byte> destination, const ReadDeadline deadline)
            -> el::stream::StreamReadResult<ByteLength> override {
            if (_state != el::stream::StreamState::Open) {
                throw StreamError{el::stream::StreamErrorContext{
                    "Failed to read from the test stream."_el, "The test input stream is closed."_el}};
            }
            deadlines.push_back(deadline);
            _readCall += 1U;
            if (_timeoutCall == _readCall) {
                return {StreamReadStatus::Timeout, ByteLength::zero()};
            }
            const auto available = _bytes.size() - _position;
            const auto count = std::min(std::min(destination.size(), available), _maximumRead);
            std::copy_n(_bytes.data() + _position, count, destination.data());
            _position += count;
            return {count == 0U ? StreamReadStatus::Finished : StreamReadStatus::Data, ByteLength::fromSizeT(count)};
        }

    public:
        using ByteInputStream::read;
        std::vector<ReadDeadline> deadlines;

    private:
        std::vector<Byte> _bytes;
        std::size_t _maximumRead{std::numeric_limits<std::size_t>::max()};
        std::optional<std::size_t> _timeoutCall;
        std::size_t _readCall{0U};
        std::size_t _position{0};
        bool _readyAtEnd{true};
        el::stream::InputStreamSettings _settings;
        el::stream::StreamState _state{el::stream::StreamState::Open};
    };

    class MemoryOutputStream final : public ByteOutputStream {
    public: // implement ByteOutputStream
        [[nodiscard]] auto outputSettings() const noexcept -> const el::stream::OutputStreamSettings & override {
            return _settings;
        }
        [[nodiscard]] auto state() const noexcept -> el::stream::StreamState override { return _state; }
        [[nodiscard]] auto isReady() const noexcept -> bool override { return _state == el::stream::StreamState::Open; }
        [[nodiscard]] auto waitForReady() -> el::stream::StreamWaitStatus override {
            return isReady() ? el::stream::StreamWaitStatus::Ready : el::stream::StreamWaitStatus::Timeout;
        }
        auto flush() -> el::stream::StreamWriteStatus override { return el::stream::StreamWriteStatus::Success; }
        auto close() -> el::stream::StreamCloseStatus override {
            _state = el::stream::StreamState::Closed;
            return el::stream::StreamCloseStatus::Closed;
        }
        void abort() noexcept override { _state = el::stream::StreamState::Closed; }

        auto write(std::span<const Byte> bytes) -> el::stream::StreamWriteStatus override {
            if (_state != el::stream::StreamState::Open) {
                throw StreamError{el::stream::StreamErrorContext{
                    "Failed to write to the test stream."_el, "The test output stream is closed."_el}};
            }
            if (timeoutWrites > 0U) {
                --timeoutWrites;
                return el::stream::StreamWriteStatus::Timeout;
            }
            for (const auto byte : bytes) {
                data.push_back(byte.toUInt8());
            }
            return el::stream::StreamWriteStatus::Success;
        }

    public:
        using ByteOutputStream::write;

    public:
        std::vector<uint8_t> data;
        std::size_t timeoutWrites{0U};

    private:
        el::stream::OutputStreamSettings _settings;
        el::stream::StreamState _state{el::stream::StreamState::Open};
    };

    static auto readLineBlocking(el::stream::impl::EncodedTextInputStream &stream, const CpLength maximum)
        -> StringEditor {
        auto result = StringEditor{};
        while (result.characterLength() < maximum) {
            const auto readResult = stream.readLine(maximum - result.characterLength());
            if (readResult == StreamReadStatus::Timeout) {
                continue;
            }
            if (readResult == StreamReadStatus::Finished) {
                break;
            }
            result.append(readResult.data());
            if (!result.isEmpty() && result.charAt(StringSide::Back) == U'\n') {
                break;
            }
        }
        return result;
    }

    static auto readAllBlocking(el::stream::impl::EncodedTextInputStream &stream) -> StringEditor {
        auto result = StringEditor{};
        while (true) {
            const auto readResult = stream.readAll();
            if (readResult == StreamReadStatus::Timeout) {
                continue;
            }
            result.append(readResult.data());
            if (readResult == StreamReadStatus::Finished) {
                return result;
            }
        }
    }

public:
    void testReadCharAndRead() {
        auto bytes = StringEncoder{StringEditor{std::string_view{"Hello"}}}.encode(StringEncoding::Utf8);
        auto byteStream = std::make_shared<MemoryInputStream>(bytes);
        auto stream = el::stream::impl::EncodedTextInputStream{byteStream, StringEncoding::Utf8};
        auto &textStream = static_cast<TextInputStream &>(stream);

        REQUIRE_EQUAL(textStream.encoding(), StringEncoding::Utf8);
        REQUIRE_EQUAL(textStream.effectiveEncoding(), StringEncoding::Utf8);
        REQUIRE_EQUAL(textStream.readChar().data(), Char{U'H'});
        REQUIRE_EQUAL(StringConverter{textStream.read(CpLength{2U}).data()}.toStdString(), std::string{"el"});
        REQUIRE_EQUAL(StringConverter{textStream.read().data()}.toStdString(), std::string{"lo"});
        REQUIRE(textStream.read() == StreamReadStatus::Finished);
    }

    void testReadLinesKeepEndingsAndTruncate() {
        auto bytes = StringEncoder{StringEditor{std::string_view{"a\r\nb\rc\nlast"}}}.encode(StringEncoding::Utf8);
        auto byteStream = std::make_shared<MemoryInputStream>(bytes);
        auto stream = el::stream::impl::EncodedTextInputStream{byteStream, StringEncoding::Utf8};
        auto &textStream = static_cast<TextInputStream &>(stream);

        REQUIRE_EQUAL(StringConverter{textStream.readLine(CpLength{2U}).data()}.toStdString(), std::string{"a\r"});
        REQUIRE_EQUAL(StringConverter{textStream.readLine(CpLength{8U}).data()}.toStdString(), std::string{"\n"});
        REQUIRE_EQUAL(StringConverter{textStream.readLine(CpLength{8U}).data()}.toStdString(), std::string{"b\rc\n"});
        REQUIRE_EQUAL(StringConverter{textStream.readAll(CpLength{2U}).data()}.toStdString(), std::string{"la"});
        REQUIRE_EQUAL(StringConverter{textStream.readAll().data()}.toStdString(), std::string{"st"});
        REQUIRE(textStream.readLine() == StreamReadStatus::Finished);
    }

    void testLineReadRetainsTextAcrossTimeout() {
        auto bytes = StringEncoder{StringEditor{std::string_view{"abc\n"}}}.encode(StringEncoding::Utf8);
        auto byteStream = std::make_shared<MemoryInputStream>(bytes, 2U, 2U);
        auto stream = el::stream::impl::EncodedTextInputStream{byteStream, StringEncoding::Utf8};

        const auto timeout = stream.readLine(CpLength{8U});
        REQUIRE(timeout.isTimeout());
        REQUIRE(timeout.data().isEmpty());

        const auto completed = stream.readLine(CpLength{8U});
        REQUIRE(completed.hasData());
        REQUIRE_EQUAL(StringConverter{completed.data()}.toStdString(), std::string{"abc\n"});
    }

    void testAggregateReadRetainsTextAcrossTimeout() {
        auto bytes = StringEncoder{StringEditor{std::string_view{"abcd"}}}.encode(StringEncoding::Utf8);
        auto byteStream = std::make_shared<MemoryInputStream>(bytes, 2U, 2U);
        auto stream = el::stream::impl::EncodedTextInputStream{byteStream, StringEncoding::Utf8};

        REQUIRE(stream.readAll(CpLength{8U}).isTimeout());
        const auto completed = stream.readAll(CpLength{8U});
        REQUIRE(completed.hasData());
        REQUIRE_EQUAL(StringConverter{completed.data()}.toStdString(), std::string{"abcd"});
    }

    void testAggregateReadsWaitForFinalStateAfterPartialData() {
        const auto bytes = StringEncoder{StringEditor{std::string_view{"last"}}}.encode(StringEncoding::Utf8);
        auto lineByteStream =
            std::make_shared<MemoryInputStream>(bytes, std::numeric_limits<std::size_t>::max(), std::nullopt, false);
        auto lineStream = el::stream::impl::EncodedTextInputStream{lineByteStream, StringEncoding::Utf8};
        REQUIRE_EQUAL(StringConverter{lineStream.readLine(CpLength{8U}).data()}.toStdString(), std::string{"last"});

        auto allByteStream =
            std::make_shared<MemoryInputStream>(bytes, std::numeric_limits<std::size_t>::max(), std::nullopt, false);
        auto allStream = el::stream::impl::EncodedTextInputStream{allByteStream, StringEncoding::Utf8};
        REQUIRE_EQUAL(StringConverter{allStream.readAll(CpLength{8U}).data()}.toStdString(), std::string{"last"});
    }

    void testLineReadUsesOneDeadlineForAllShortRefills() {
        auto bytes = StringEncoder{StringEditor{std::string_view{"abc\n"}}}.encode(StringEncoding::Utf8);
        auto byteStream = std::make_shared<MemoryInputStream>(bytes, 1U);
        auto stream = el::stream::impl::EncodedTextInputStream{byteStream, StringEncoding::Utf8};

        REQUIRE(stream.readLine(CpLength{8U}).hasData());
        REQUIRE_EQUAL(byteStream->deadlines.size(), std::size_t{4U});
        REQUIRE(
            std::all_of(byteStream->deadlines.begin(), byteStream->deadlines.end(), [&byteStream](const auto deadline) {
                return deadline == byteStream->deadlines.front();
            }));
    }

    void testChangingTextReadReplaysRetainedText() {
        auto bytes = StringEncoder{StringEditor{std::string_view{"abc\n"}}}.encode(StringEncoding::Utf8);
        auto byteStream = std::make_shared<MemoryInputStream>(bytes, 2U, 2U);
        auto stream = el::stream::impl::EncodedTextInputStream{byteStream, StringEncoding::Utf8};

        REQUIRE(stream.readLine(CpLength{8U}).isTimeout());
        REQUIRE_EQUAL(StringConverter{stream.read(CpLength{2U}).data()}.toStdString(), std::string{"ab"});
        REQUIRE_EQUAL(StringConverter{stream.readLine(CpLength{8U}).data()}.toStdString(), std::string{"c\n"});
    }

    void testMixedInputReadsPreserveOrder() {
        auto bytes = StringEncoder{StringEditor{std::string_view{"A\nBC\nD"}}}.encode(StringEncoding::Utf8);
        auto byteStream = std::make_shared<MemoryInputStream>(bytes, 2U);
        auto stream = el::stream::impl::EncodedTextInputStream{byteStream, StringEncoding::Utf8};

        REQUIRE_EQUAL(stream.readChar().data(), Char{U'A'});
        REQUIRE_EQUAL(StringConverter{readLineBlocking(stream, CpLength{8U})}.toStdString(), std::string{"\n"});
        auto middle = StringEditor{};
        while (middle.characterLength() < CpLength{2U}) {
            const auto value = stream.read(CpLength{2U} - middle.characterLength());
            if (value == StreamReadStatus::Data) {
                middle.append(value.data());
            }
        }
        REQUIRE_EQUAL(StringConverter{middle}.toStdString(), std::string{"BC"});
        REQUIRE_EQUAL(StringConverter{readLineBlocking(stream, CpLength{8U})}.toStdString(), std::string{"\n"});
        REQUIRE_EQUAL(StringConverter{readAllBlocking(stream)}.toStdString(), std::string{"D"});
        REQUIRE(stream.readChar() == StreamReadStatus::Finished);
    }

    void testZeroMaximumReturnsEmptyWithoutAdvancing() {
        auto bytes = StringEncoder{StringEditor{std::string_view{"abc"}}}.encode(StringEncoding::Utf8);
        auto byteStream = std::make_shared<MemoryInputStream>(bytes);
        auto stream = el::stream::impl::EncodedTextInputStream{byteStream, StringEncoding::Utf8};

        REQUIRE(stream.read(CpLength::zero()).data().isEmpty());
        REQUIRE_EQUAL(StringConverter{stream.readAll(CpLength{3U}).data()}.toStdString(), std::string{"abc"});
    }

    void testAllocatingReadsRejectInfiniteMaximum() {
        auto bytes = StringEncoder{StringEditor{std::string_view{"A"}}}.encode(StringEncoding::Utf8);
        auto byteStream = std::make_shared<MemoryInputStream>(bytes);
        auto stream = el::stream::impl::EncodedTextInputStream{byteStream, StringEncoding::Utf8};

        REQUIRE_THROWS_AS(el::err::ParameterError, stream.read(CpLength::infinite()));
        REQUIRE_THROWS_AS(el::err::ParameterError, stream.readLine(CpLength::infinite()));
        REQUIRE_THROWS_AS(el::err::ParameterError, stream.readAll(CpLength::infinite()));
    }

    void testBomAndEffectiveEncoding() {
        auto bytes = StringEncoder{StringEditor{std::string_view{"A"}}}.encode(StringEncoding::Utf16);
        auto byteStream = std::make_shared<MemoryInputStream>(bytes);
        auto stream = el::stream::impl::EncodedTextInputStream{byteStream, StringEncoding::Utf16};

        REQUIRE_EQUAL(stream.effectiveEncoding(), StringEncoding::Utf16LittleEndian);
        REQUIRE_EQUAL(StringConverter{stream.readAll().data()}.toStdString(), std::string{"A"});
        REQUIRE_EQUAL(stream.effectiveEncoding(), StringEncoding::Utf16LittleEndian);
    }

    void testThrowModePropagatesEncodingErrors() {
        auto bytes = ByteBlock{std::vector<uint8_t>{0xffU}};
        auto byteStream = std::make_shared<MemoryInputStream>(bytes);
        auto stream = el::stream::impl::EncodedTextInputStream{
            byteStream, StringEncoding::Utf8, StringBomMode::Automatic, EncodingErrorMode::Throw};

        REQUIRE_THROWS_AS(EncodingError, stream.readAll());
    }

    void testInputUtf8SplitSequences() {
        auto bytes = ByteBlock{std::vector<uint8_t>{0x41U, 0xF0U, 0x9FU, 0x98U, 0x80U, 0x0DU, 0x0AU, 0x42U}};
        auto byteStream = std::make_shared<MemoryInputStream>(bytes, 1U);
        auto stream = el::stream::impl::EncodedTextInputStream{byteStream, StringEncoding::Utf8};

        REQUIRE_EQUAL(
            StringConverter{readLineBlocking(stream, CpLength{8U})}.toStdString(),
            std::string{"A"} + th::stdStringFromHex("F0 9F 98 80") + "\r\n");
        REQUIRE_EQUAL(StringConverter{readAllBlocking(stream)}.toStdString(), std::string{"B"});
        REQUIRE(stream.read() == StreamReadStatus::Finished);
    }

    void testInputUtf16SplitSequences() {
        auto bytes = ByteBlock{
            std::vector<uint8_t>{0xFFU, 0xFEU, 0x41U, 0x00U, 0x3DU, 0xD8U, 0x00U, 0xDEU, 0x0AU, 0x00U, 0x42U, 0x00U}};
        auto byteStream = std::make_shared<MemoryInputStream>(bytes, 1U);
        auto stream = el::stream::impl::EncodedTextInputStream{byteStream, StringEncoding::Utf16};

        REQUIRE_EQUAL(
            StringConverter{readLineBlocking(stream, CpLength{8U})}.toStdString(),
            std::string{"A"} + th::stdStringFromHex("F0 9F 98 80") + "\n");
        REQUIRE_EQUAL(StringConverter{readAllBlocking(stream)}.toStdString(), std::string{"B"});
        REQUIRE(stream.read() == StreamReadStatus::Finished);
    }

    void testOutputUtf8AndBomOnlyOnce() {

        const auto byteStream = std::make_shared<MemoryOutputStream>();
        auto stream = el::stream::impl::EncodedTextOutputStream{byteStream, StringEncoding::Utf16};

        stream.print("A");
        stream.writeLine("B"_el);

        REQUIRE_EQUAL(byteStream->data, std::vector<uint8_t>({0xffU, 0xfeU, 0x41U, 0x00U, 0x42U, 0x00U, 0x0aU, 0x00U}));
        REQUIRE_EQUAL(stream.encoding(), StringEncoding::Utf16);
        REQUIRE_EQUAL(stream.effectiveEncoding(), StringEncoding::Utf16LittleEndian);
    }

    void testOutputRequiredUtf8BomOnlyOnce() {
        const auto byteStream = std::make_shared<MemoryOutputStream>();
        auto stream =
            el::stream::impl::EncodedTextOutputStream{byteStream, StringEncoding::Utf8, StringBomMode::Require};

        REQUIRE(stream.write("A"_el).isSuccess());
        REQUIRE(stream.write(Char{U'B'}).isSuccess());

        REQUIRE_EQUAL(byteStream->data, std::vector<uint8_t>({0xefU, 0xbbU, 0xbfU, 0x41U, 0x42U}));
    }

    void testOutputRequiredUtf32BomOnlyOnce() {
        const auto byteStream = std::make_shared<MemoryOutputStream>();
        auto stream = el::stream::impl::EncodedTextOutputStream{
            byteStream, StringEncoding::Utf32BigEndian, StringBomMode::Require};

        stream.print("A");
        REQUIRE(stream.writeLine("B"_el).isSuccess());

        REQUIRE_EQUAL(
            byteStream->data,
            std::vector<uint8_t>(
                {0x00U,
                    0x00U,
                    0xfeU,
                    0xffU,
                    0x00U,
                    0x00U,
                    0x00U,
                    0x41U,
                    0x00U,
                    0x00U,
                    0x00U,
                    0x42U,
                    0x00U,
                    0x00U,
                    0x00U,
                    0x0aU}));
    }

    void testTimedOutFirstWriteRetriesBomOnce() {
        const auto byteStream = std::make_shared<MemoryOutputStream>();
        byteStream->timeoutWrites = 1U;
        auto stream =
            el::stream::impl::EncodedTextOutputStream{byteStream, StringEncoding::Utf16, StringBomMode::Require};

        REQUIRE(stream.write("A"_el).isTimeout());
        REQUIRE(byteStream->data.empty());
        REQUIRE(stream.write("A"_el).isSuccess());
        REQUIRE(stream.write("B"_el).isSuccess());

        REQUIRE_EQUAL(byteStream->data, std::vector<uint8_t>({0xffU, 0xfeU, 0x41U, 0x00U, 0x42U, 0x00U}));
    }

    void testOutputCanSuppressInitialBom() {

        const auto byteStream = std::make_shared<MemoryOutputStream>();
        auto stream = el::stream::impl::EncodedTextOutputStream{
            byteStream, StringEncoding::Utf16, StringBomMode::Require, EncodingErrorMode::Replace, true};

        stream.write("A"_el);

        REQUIRE_EQUAL(byteStream->data, std::vector<uint8_t>({0x41U, 0x00U}));
    }
};
