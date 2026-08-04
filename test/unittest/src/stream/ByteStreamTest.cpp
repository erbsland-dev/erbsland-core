// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/ParameterError.hpp>
#include <erbsland/stream/ByteInputStream.hpp>
#include <erbsland/stream/ByteOutputStream.hpp>
#include <erbsland/stream/StreamError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <algorithm>
#include <chrono>
#include <limits>
#include <optional>
#include <vector>

using el::mem::Byte;
using el::mem::Endianness;
using el::stream::ByteInputStream;
using el::stream::ByteOutputStream;
using el::stream::StreamError;
using el::stream::StreamReadStatus;
using el::unit::ByteLength;
using namespace el::text::literals;

TESTED_TARGETS(ByteInputStream ByteOutputStream)
class ByteStreamTest final : public el::UnitTest {
    class MemoryInputStream final : public ByteInputStream {
    public:
        explicit MemoryInputStream(
            std::vector<uint8_t> bytes,
            const std::size_t maximumRead = std::numeric_limits<std::size_t>::max(),
            const std::optional<std::size_t> timeoutCall = std::nullopt,
            const bool sensitive = false) :
            _maximumRead{maximumRead}, _timeoutCall{timeoutCall} {
            _settings.setSensitive(sensitive);
            _bytes.reserve(bytes.size());
            for (const auto byte : bytes) {
                _bytes.push_back(Byte{byte});
            }
        }

    public: // implement ByteInputStream
        [[nodiscard]] auto inputSettings() const noexcept -> const el::stream::InputStreamSettings & override {
            return _settings;
        }
        [[nodiscard]] auto state() const noexcept -> el::stream::StreamState override { return _state; }
        [[nodiscard]] auto isReady() const noexcept -> bool override { return true; }
        [[nodiscard]] auto waitForReady() -> el::stream::StreamWaitStatus override {
            return el::stream::StreamWaitStatus::Ready;
        }
        auto close() -> el::stream::StreamCloseStatus override {
            _state = el::stream::StreamState::Closed;
            return el::stream::StreamCloseStatus::Closed;
        }
        void abort() noexcept override { _state = el::stream::StreamState::Closed; }

    protected:
        [[nodiscard]] auto readFromSource(el::mem::ByteSpan destination, const ReadDeadline deadline)
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
        std::vector<ReadDeadline> deadlines;

    private:
        std::vector<Byte> _bytes;
        std::size_t _maximumRead;
        std::optional<std::size_t> _timeoutCall;
        std::size_t _readCall{0U};
        std::size_t _position{0};
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
        auto flush() -> el::stream::StreamWriteStatus override {
            flushCount += 1U;
            return el::stream::StreamWriteStatus::Success;
        }
        auto close() -> el::stream::StreamCloseStatus override {
            _state = el::stream::StreamState::Closed;
            return el::stream::StreamCloseStatus::Closed;
        }
        void abort() noexcept override { _state = el::stream::StreamState::Closed; }

        auto write(el::mem::ConstByteSpan bytes) -> el::stream::StreamWriteStatus override {
            if (_state != el::stream::StreamState::Open) {
                throw StreamError{el::stream::StreamErrorContext{
                    "Failed to write to the test stream."_el, "The test output stream is closed."_el}};
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
        std::size_t flushCount{0};

    private:
        el::stream::OutputStreamSettings _settings;
        el::stream::StreamState _state{el::stream::StreamState::Open};
    };

public:
    void testInputConvenienceMethods() {
        auto stream = MemoryInputStream{{0x34U, 0x12U, 0xabU, 0xcdU, 0xffU}};

        const auto byte = stream.readByte();
        REQUIRE_EQUAL(byte, StreamReadStatus::Data);
        REQUIRE_EQUAL(byte.data(), Byte{0x34U});
        REQUIRE_EQUAL(stream.readUInt8().data(), uint8_t{0x12U});

        stream.setEndianness(Endianness::Big);
        REQUIRE_EQUAL(stream.readUInt16().data(), uint16_t{0xabcdU});
        REQUIRE_EQUAL(stream.readInt8().data(), int8_t{-1});
        const auto end = stream.readByte();
        REQUIRE_EQUAL(end, StreamReadStatus::Finished);
    }

    void testInputExactReads() {
        auto stream = MemoryInputStream{{1U, 2U, 3U}};

        const auto first = stream.readExact(ByteLength{2U});
        REQUIRE_EQUAL(first, StreamReadStatus::Data);
        REQUIRE_EQUAL(first.data().toUInt8Vector(), std::vector<uint8_t>({1U, 2U}));

        const auto second = stream.readExact(ByteLength{2U});
        REQUIRE_EQUAL(second, StreamReadStatus::Finished);
        REQUIRE(second.data().isEmpty());
        REQUIRE_EQUAL(stream.readByte().data(), Byte{3U});
        const auto end = stream.readByte();
        REQUIRE_EQUAL(end, StreamReadStatus::Finished);
    }

    void testExactReadRetainsDataAcrossTimeout() {
        auto stream = MemoryInputStream{{1U, 2U}, 1U, 2U};

        const auto timeout = stream.readExact(ByteLength{2U});
        REQUIRE(timeout.isTimeout());
        REQUIRE(timeout.data().isEmpty());

        const auto completed = stream.readExact(ByteLength{2U});
        REQUIRE(completed.hasData());
        REQUIRE_EQUAL(completed.data().toUInt8Vector(), std::vector<uint8_t>({1U, 2U}));
    }

    void testExactReadUsesOneDeadlineForAllShortReads() {
        auto stream = MemoryInputStream{{1U, 2U, 3U}, 1U};

        const auto exact = stream.readExact(ByteLength{3U});
        REQUIRE(exact.hasData());
        REQUIRE_EQUAL(stream.deadlines.size(), std::size_t{3U});
        REQUIRE(std::all_of(stream.deadlines.begin(), stream.deadlines.end(), [&stream](const auto deadline) {
            return deadline == stream.deadlines.front();
        }));
    }

    void testChangingReadReplaysRetainedData() {
        auto stream = MemoryInputStream{{1U, 2U}, 1U, 2U};

        const auto timeout = stream.readExact(ByteLength{2U});
        REQUIRE(timeout.isTimeout());
        const auto first = stream.read(ByteLength{1U});
        const auto second = stream.read(ByteLength{1U});
        REQUIRE_EQUAL(first.data().toUInt8Vector(), std::vector<uint8_t>({1U}));
        REQUIRE_EQUAL(second.data().toUInt8Vector(), std::vector<uint8_t>({2U}));
    }

    void testInputMaximumRead() {
        auto stream = MemoryInputStream{{1U, 2U, 3U}};

        const auto first = stream.read(ByteLength{2U});
        const auto second = stream.read(ByteLength{2U});
        const auto end = stream.read(ByteLength{2U});
        REQUIRE_EQUAL(first.data().toUInt8Vector(), std::vector<uint8_t>({1U, 2U}));
        REQUIRE_EQUAL(second.data().toUInt8Vector(), std::vector<uint8_t>({3U}));
        REQUIRE_EQUAL(end, StreamReadStatus::Finished);
    }

    void testInputIntegerPartialIsRetained() {
        auto stream = MemoryInputStream{{0x34U}};

        const auto integer = stream.readUInt16();
        REQUIRE_EQUAL(integer, StreamReadStatus::Finished);
        REQUIRE_EQUAL(stream.readByte().data(), Byte{0x34U});
        const auto end = stream.readByte();
        REQUIRE_EQUAL(end, StreamReadStatus::Finished);
    }

    void testReadAll() {
        auto stream = MemoryInputStream{{1U, 2U, 3U}};

        const auto first = stream.readAll(ByteLength{2U});
        REQUIRE_EQUAL(first, StreamReadStatus::Data);
        REQUIRE_EQUAL(first.data().toUInt8Vector(), std::vector<uint8_t>({1U, 2U}));
        const auto remaining = stream.readAll();
        const auto end = stream.readAll();
        REQUIRE_EQUAL(remaining.data().toUInt8Vector(), std::vector<uint8_t>({3U}));
        REQUIRE(end.data().isEmpty());
    }

    void testAllocatingReadsRejectInfiniteMaximum() {
        auto stream = MemoryInputStream{{1U}};

        REQUIRE_THROWS_AS(el::err::ParameterError, stream.read(ByteLength::infinite()));
        REQUIRE_THROWS_AS(el::err::ParameterError, stream.readExact(ByteLength::infinite()));
        REQUIRE_THROWS_AS(el::err::ParameterError, stream.readAll(ByteLength::infinite()));
    }

    void testOrdinaryOwnedReadsAndTimeoutContinuation() {
        auto stream = MemoryInputStream{{1U, 2U, 3U}, 1U, 2U};

        const auto timeout = stream.readExact(ByteLength{2U});
        REQUIRE(timeout.isTimeout());
        const auto exact = stream.readExact(ByteLength{2U});
        REQUIRE(exact.hasData());
        REQUIRE_EQUAL(exact.data(), el::mem::ByteBlock({1U, 2U}));
        REQUIRE_FALSE(exact.data().isSensitive());

        const auto all = stream.readAll();
        REQUIRE(all.hasData());
        REQUIRE_EQUAL(all.data(), el::mem::ByteBlock({3U}));
        REQUIRE_FALSE(all.data().isSensitive());
        const auto empty = stream.read(ByteLength::zero());
        REQUIRE_FALSE(empty.data().isSensitive());
    }

    void testSensitivePolicyMarksAllOwnedResultsAndOperationSwitches() {
        auto stream = MemoryInputStream{{1U, 2U, 3U, 4U}, 1U, 2U, true};

        const auto timeout = stream.readExact(ByteLength{2U});
        REQUIRE(timeout.isTimeout());
        REQUIRE(stream.inputSettings().isSensitive());
        const auto protectedPrefix = stream.read(ByteLength{1U});
        REQUIRE_EQUAL(protectedPrefix.data(), el::mem::ByteBlock({1U}));
        REQUIRE(protectedPrefix.data().isSensitive());
        const auto middle = stream.read(ByteLength{2U});
        REQUIRE_EQUAL(middle.data().toUInt8Vector(), std::vector<uint8_t>({2U}));
        REQUIRE(middle.data().isSensitive());
        const auto protectedSuffix = stream.readAll();
        REQUIRE_EQUAL(protectedSuffix.data(), el::mem::ByteBlock({3U, 4U}));
        REQUIRE(protectedSuffix.data().isSensitive());
    }

    void testOutputConvenienceMethods() {
        auto stream = MemoryOutputStream{};

        stream.write(Byte{1U});
        stream.writeUInt16(0x1234U);
        stream.setEndianness(Endianness::Big);
        stream.writeUInt16(0xabcdU);
        stream.writeInt8(-1);
        stream.flush();

        REQUIRE_EQUAL(stream.data, std::vector<uint8_t>({1U, 0x34U, 0x12U, 0xabU, 0xcdU, 0xffU}));
        REQUIRE_EQUAL(stream.flushCount, std::size_t{1U});
    }
};
