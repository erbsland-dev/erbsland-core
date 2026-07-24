// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/LogicError.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/stream/AnyStringBuilderStream.hpp>
#include <erbsland/stream/ByteInputStream.hpp>
#include <erbsland/stream/ByteOutputStream.hpp>
#include <erbsland/stream/impl/EncodedTextInputStream.hpp>
#include <erbsland/stream/StreamError.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringEncoder.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <limits>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

using el::mem::Byte;
using el::mem::ByteBlock;
using el::stream::ByteInputStream;
using el::stream::ByteOutputStream;
using el::stream::StreamError;
using el::stream::StreamReadResult;
using el::stream::StreamReadStatus;
using el::stream::StreamWriteStatus;
using el::text::StringConverter;
using el::text::StringEditor;
using el::text::StringEncoder;
using el::text::StringEncoding;
using el::unit::ByteLength;
using el::unit::CpLength;
using el::util::CoAsyncGenerator;
using el::util::CoTask;
using namespace el::text::literals;

namespace erbsland::test::asyncstreamtest {

template <typename tValue>
void waitFor(CoTask<tValue> &task) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
    while (!task.isComplete() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::yield();
    }
}

template <typename tPredicate>
void waitUntil(tPredicate predicate) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
    while (!predicate() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::yield();
    }
}

template <typename tValue>
[[nodiscard]] auto nextValue(CoAsyncGenerator<tValue> &generator) -> CoTask<std::optional<tValue>> {
    co_return co_await generator.next();
}

class ControlledByteInputStream final : public ByteInputStream {
public:
    explicit ControlledByteInputStream(
        const ByteBlock &bytes,
        const std::size_t maximumRead = std::numeric_limits<std::size_t>::max(),
        const std::size_t timeoutCount = 0U,
        const bool blocked = false,
        const bool sensitive = false) :
        _bytes{bytes.span().begin(), bytes.span().end()},
        _maximumRead{maximumRead},
        _timeoutsRemaining{timeoutCount},
        _blocked{blocked} {
        _settings.setSensitive(sensitive);
    }

public: // controls
    void release() noexcept { _released.store(true); }
    [[nodiscard]] auto started() const noexcept -> bool { return _started.load(); }
    [[nodiscard]] auto sourceReadCompleted() const noexcept -> bool { return _sourceReadCompleted.load(); }

public: // implement ByteInputStream
    [[nodiscard]] auto inputSettings() const noexcept -> const el::stream::InputStreamSettings & override {
        return _settings;
    }
    [[nodiscard]] auto state() const noexcept -> el::stream::StreamState override { return _state; }
    [[nodiscard]] auto isReady() const noexcept -> bool override { return _state == el::stream::StreamState::Open; }
    [[nodiscard]] auto waitForReady() -> el::stream::StreamWaitStatus override {
        return isReady() ? el::stream::StreamWaitStatus::Ready : el::stream::StreamWaitStatus::Timeout;
    }
    auto close() -> el::stream::StreamCloseStatus override {
        _state = el::stream::StreamState::Closed;
        return el::stream::StreamCloseStatus::Closed;
    }
    void abort() noexcept override { _state = el::stream::StreamState::Closed; }

protected:
    [[nodiscard]] auto readFromSource(el::mem::ByteSpan destination, ReadDeadline)
        -> StreamReadResult<ByteLength> override {
        if (_state != el::stream::StreamState::Open) {
            throw StreamError{el::stream::StreamErrorContext{
                "Failed to read from the controlled stream."_el, "The controlled stream is closed."_el}};
        }
        _started.store(true);
        while (_blocked && !_released.load()) {
            std::this_thread::yield();
        }
        if (_timeoutsRemaining > 0U) {
            _timeoutsRemaining -= 1U;
            _sourceReadCompleted.store(true);
            return {StreamReadStatus::Timeout, ByteLength::zero()};
        }
        const auto count = std::min({destination.size(), _bytes.size() - _position, _maximumRead});
        std::copy_n(_bytes.data() + _position, count, destination.data());
        _position += count;
        _sourceReadCompleted.store(true);
        return {count == 0U ? StreamReadStatus::Finished : StreamReadStatus::Data, ByteLength::fromSizeT(count)};
    }

private:
    std::vector<Byte> _bytes;
    std::size_t _maximumRead;
    std::size_t _timeoutsRemaining;
    std::size_t _position{0U};
    bool _blocked;
    std::atomic<bool> _released{false};
    std::atomic<bool> _started{false};
    std::atomic<bool> _sourceReadCompleted{false};
    el::stream::InputStreamSettings _settings;
    el::stream::StreamState _state{el::stream::StreamState::Open};
};

class ControlledByteOutputStream final : public ByteOutputStream {
public:
    explicit ControlledByteOutputStream(
        const StreamWriteStatus result = StreamWriteStatus::Success, const bool blocked = false) :
        _result{result}, _blocked{blocked} {}

public: // controls
    void release() noexcept { _released.store(true); }
    [[nodiscard]] auto started() const noexcept -> bool { return _started.load(); }
    [[nodiscard]] auto data() const -> std::vector<uint8_t> { return _data; }

public: // implement ByteOutputStream
    [[nodiscard]] auto outputSettings() const noexcept -> const el::stream::OutputStreamSettings & override {
        return _settings;
    }
    [[nodiscard]] auto state() const noexcept -> el::stream::StreamState override { return _state; }
    [[nodiscard]] auto isReady() const noexcept -> bool override { return _state == el::stream::StreamState::Open; }
    [[nodiscard]] auto waitForReady() -> el::stream::StreamWaitStatus override {
        return isReady() ? el::stream::StreamWaitStatus::Ready : el::stream::StreamWaitStatus::Timeout;
    }
    auto flush() -> StreamWriteStatus override { return StreamWriteStatus::Success; }
    auto close() -> el::stream::StreamCloseStatus override {
        _state = el::stream::StreamState::Closed;
        return el::stream::StreamCloseStatus::Closed;
    }
    void abort() noexcept override { _state = el::stream::StreamState::Closed; }
    auto write(const el::mem::ConstByteSpan bytes) -> StreamWriteStatus override {
        if (_state != el::stream::StreamState::Open) {
            throw StreamError{el::stream::StreamErrorContext{
                "Failed to write to the controlled stream."_el, "The controlled stream is closed."_el}};
        }
        _started.store(true);
        while (_blocked && !_released.load()) {
            std::this_thread::yield();
        }
        if (_result == StreamWriteStatus::Success) {
            for (const auto byte : bytes) {
                _data.push_back(byte.toUInt8());
            }
        }
        return _result;
    }

private:
    StreamWriteStatus _result;
    bool _blocked;
    std::atomic<bool> _released{false};
    std::atomic<bool> _started{false};
    std::vector<uint8_t> _data;
    el::stream::OutputStreamSettings _settings;
    el::stream::StreamState _state{el::stream::StreamState::Open};
};

[[nodiscard]] static auto createTextInput(const text::String &text, const StringEncoding encoding)
    -> std::shared_ptr<el::stream::impl::EncodedTextInputStream> {
    const auto bytes = StringEncoder{text}.encode(encoding);
    auto byteStream = std::make_shared<ControlledByteInputStream>(bytes, 1U);
    return std::make_shared<el::stream::impl::EncodedTextInputStream>(byteStream, encoding);
}

}

using namespace erbsland::test::asyncstreamtest;

TESTED_TARGETS(ByteInputStream ByteOutputStream TextInputStream TextOutputStream)
class AsyncStreamTest final : public el::UnitTest {
public:
    void testByteExactAndAllPreserveStatuses() {
        auto stream = std::make_shared<ControlledByteInputStream>(
            ByteBlock::fromVector(std::vector<uint8_t>{1U, 2U, 3U, 4U}), 1U, 1U);

        auto timeout = stream->coReadExact(ByteLength{2U});
        waitFor(timeout);
        REQUIRE(timeout.isComplete());
        REQUIRE(timeout.result().isTimeout());
        REQUIRE(timeout.result().data().isEmpty());

        auto exact = stream->coReadExact(ByteLength{2U});
        waitFor(exact);
        REQUIRE_EQUAL(exact.result().data().toUInt8Vector(), std::vector<uint8_t>({1U, 2U}));

        auto all = stream->coReadAll(ByteLength{8U});
        waitFor(all);
        REQUIRE_EQUAL(all.result().data().toUInt8Vector(), std::vector<uint8_t>({3U, 4U}));

        auto finished = stream->coReadAll(ByteLength{8U});
        waitFor(finished);
        REQUIRE(finished.result().isFinished());
    }

    void testSensitiveCoroutineReadsAreMarked() {
        auto stream = std::make_shared<ControlledByteInputStream>(
            ByteBlock::fromVector(std::vector<uint8_t>{1U, 2U, 3U}), 1U, 1U, false, true);

        auto timeout = stream->coReadExact(ByteLength{2U});
        waitFor(timeout);
        REQUIRE(timeout.result().isTimeout());

        auto exact = stream->coReadExact(ByteLength{2U});
        waitFor(exact);
        REQUIRE_EQUAL(exact.result().data().toUInt8Vector(), std::vector<uint8_t>({1U, 2U}));
        REQUIRE(exact.result().data().isSensitive());

        auto all = stream->coReadAll(ByteLength{8U});
        waitFor(all);
        REQUIRE_EQUAL(all.result().data().toUInt8Vector(), std::vector<uint8_t>({3U}));
        REQUIRE(all.result().data().isSensitive());
    }

    void testReadDoesNotBlockCallerAndRetainsStream() {
        auto stream = std::make_shared<ControlledByteInputStream>(
            ByteBlock::fromVector(std::vector<uint8_t>{42U}), std::numeric_limits<std::size_t>::max(), 0U, true);
        auto weakStream = std::weak_ptr<ControlledByteInputStream>{stream};

        auto task = stream->coRead(ByteLength{1U});
        stream.reset();
        waitUntil([&weakStream]() -> bool { return weakStream.lock()->started(); });
        REQUIRE_FALSE(task.isComplete());
        REQUIRE_FALSE(weakStream.expired());

        weakStream.lock()->release();
        waitFor(task);
        REQUIRE_EQUAL(task.result().data().toUInt8Vector(), std::vector<uint8_t>({42U}));
        waitUntil([&weakStream]() -> bool { return weakStream.expired(); });
        REQUIRE(weakStream.expired());
    }

    void testTaskDestructionCancelsContinuation() {
        auto stream = std::make_shared<ControlledByteInputStream>(
            ByteBlock::fromVector(std::vector<uint8_t>{9U}), std::numeric_limits<std::size_t>::max(), 0U, true);
        auto weakStream = std::weak_ptr<ControlledByteInputStream>{stream};
        {
            auto task = stream->coRead(ByteLength{1U});
            waitUntil([&stream]() -> bool { return stream->started(); });
            REQUIRE_FALSE(task.isComplete());
        }
        stream->release();
        waitUntil([&stream]() -> bool { return stream->sourceReadCompleted(); });
        stream.reset();
        waitUntil([&weakStream]() -> bool { return weakStream.expired(); });
        REQUIRE(weakStream.expired());
    }

    void testByteBlockGeneratorYieldsTimeoutAndData() {
        auto stream = std::make_shared<ControlledByteInputStream>(
            ByteBlock::fromVector(std::vector<uint8_t>{7U, 8U}), std::numeric_limits<std::size_t>::max(), 1U);
        auto generator = stream->coReadBlocks(ByteLength{8U});

        auto timeout = nextValue(generator);
        waitFor(timeout);
        REQUIRE(timeout.result().has_value());
        REQUIRE(timeout.result()->isTimeout());

        auto data = nextValue(generator);
        waitFor(data);
        REQUIRE_EQUAL(data.result()->data().toUInt8Vector(), std::vector<uint8_t>({7U, 8U}));

        auto finished = nextValue(generator);
        waitFor(finished);
        REQUIRE_FALSE(finished.result().has_value());
    }

    void testRejectsStackOwnershipAndPropagatesErrors() {
        auto stackStream = ControlledByteInputStream{ByteBlock::fromVector(std::vector<uint8_t>{1U})};
        REQUIRE_THROWS_AS(el::err::LogicError, stackStream.coRead(ByteLength{1U}));

        auto stream = std::make_shared<ControlledByteInputStream>(ByteBlock::fromVector(std::vector<uint8_t>{1U}));
        stream->close();
        auto task = stream->coRead(ByteLength{1U});
        waitFor(task);
        REQUIRE_THROWS_AS(StreamError, task.result());
    }

    void testOwnedByteAndTextWrites() {
        auto byteStream = std::make_shared<ControlledByteOutputStream>(StreamWriteStatus::Success, true);
        auto writeTask = byteStream->coWrite(ByteBlock::fromVector(std::vector<uint8_t>{1U, 2U, 3U}));
        waitUntil([&byteStream]() -> bool { return byteStream->started(); });
        REQUIRE_FALSE(writeTask.isComplete());
        byteStream->release();
        waitFor(writeTask);
        REQUIRE(writeTask.result().isSuccess());
        REQUIRE_EQUAL(byteStream->data(), std::vector<uint8_t>({1U, 2U, 3U}));

        auto timeoutStream = std::make_shared<ControlledByteOutputStream>(StreamWriteStatus::Timeout);
        auto timeoutTask = timeoutStream->coWrite(ByteBlock::fromVector(std::vector<uint8_t>{4U, 5U}));
        waitFor(timeoutTask);
        REQUIRE(timeoutTask.result().isTimeout());
        REQUIRE(timeoutStream->data().empty());

        auto textStream = el::stream::AnyStringBuilderStream::create();
        auto textTask = textStream->coWrite(StringEditor{std::u8string_view{u8"A😀"}});
        waitFor(textTask);
        REQUIRE(textTask.result().isSuccess());
        auto lineTask = textStream->coWriteLine(StringEditor{std::string_view{"B"}});
        waitFor(lineTask);
        REQUIRE(lineTask.result().isSuccess());
        auto emptyLineTask = textStream->coWriteLine();
        waitFor(emptyLineTask);
        REQUIRE(emptyLineTask.result().isSuccess());
        REQUIRE_EQUAL(StringConverter{textStream->toString()}.toStdU8String(), std::u8string{u8"A😀B\n\n"});
    }

    void testTextBlocksPreserveCodePointBoundaries() {
        for (const auto encoding : {StringEncoding::Utf8, StringEncoding::Utf16}) {
            auto stream = createTextInput(StringEditor{std::u8string_view{u8"A😀B"}}, encoding);
            auto generator = stream->coReadBlocks(CpLength{1U});

            auto first = nextValue(generator);
            waitFor(first);
            REQUIRE_EQUAL(StringConverter{first.result()->data()}.toStdU8String(), std::u8string{u8"A"});
            auto second = nextValue(generator);
            waitFor(second);
            REQUIRE_EQUAL(StringConverter{second.result()->data()}.toStdU8String(), std::u8string{u8"😀"});
            auto third = nextValue(generator);
            waitFor(third);
            REQUIRE_EQUAL(StringConverter{third.result()->data()}.toStdU8String(), std::u8string{u8"B"});
            auto finished = nextValue(generator);
            waitFor(finished);
            REQUIRE_FALSE(finished.result().has_value());
        }
    }

    void testTextLineGeneratorPreservesEndings() {
        auto stream = createTextInput(StringEditor{std::u8string_view{u8"A😀\r\nB"}}, StringEncoding::Utf8);
        auto generator = stream->coReadLines(CpLength{8U});

        auto first = nextValue(generator);
        waitFor(first);
        REQUIRE_EQUAL(StringConverter{first.result()->data()}.toStdU8String(), std::u8string{u8"A😀\r\n"});
        auto second = nextValue(generator);
        waitFor(second);
        REQUIRE_EQUAL(StringConverter{second.result()->data()}.toStdU8String(), std::u8string{u8"B"});
        auto finished = nextValue(generator);
        waitFor(finished);
        REQUIRE_FALSE(finished.result().has_value());
    }
};
