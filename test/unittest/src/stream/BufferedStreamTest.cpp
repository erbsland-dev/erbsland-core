// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/stream/impl/BufferedByteInputStream.hpp>
#include <erbsland/stream/impl/BufferedByteInputStreamData.hpp>
#include <erbsland/stream/impl/BufferedByteOutputStream.hpp>
#include <erbsland/stream/impl/BufferedByteOutputStreamData.hpp>
#include <erbsland/stream/impl/EncodedTextOutputStream.hpp>
#include <erbsland/stream/impl/IoService.hpp>
#include <erbsland/stream/impl/NativeByteStream.hpp>
#include <erbsland/stream/StreamError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringBomMode.hpp>
#include <erbsland/text/StringEncoding.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <span>
#include <thread>
#include <vector>

using el::mem::Byte;
using el::stream::StreamCloseStatus;
using el::stream::StreamPositionOrigin;
using el::stream::StreamReadStatus;
using el::stream::StreamWaitStatus;
using el::stream::StreamWriteStatus;
using el::unit::ByteLength;
using el::unit::ByteOffset;
using namespace el::text::literals;

TESTED_TARGETS(
    BufferedByteInputStream BufferedByteInputStreamData BufferedByteOutputStream BufferedByteOutputStreamData IoService
        StreamState StreamReadStatus StreamWriteStatus StreamCloseStatus StreamWaitStatus InputStreamSettings
            OutputStreamSettings)
class BufferedStreamTest final : public el::UnitTest {
    class NativeStream final : public el::stream::impl::NativeByteStream {
    public:
        [[nodiscard]] auto supportsPositioning() const noexcept -> bool override { return positionable; }
        [[nodiscard]] auto position() const -> el::unit::ByteIndex override {
            if (!positionable) {
                throw el::stream::StreamError{el::stream::StreamErrorContext{
                    "Failed to get the test stream position."_el, "The native test stream is not positionable."_el}};
            }
            return el::unit::ByteIndex::fromSizeT(streamPosition);
        }
        auto setPosition(const el::unit::ByteIndex position) -> el::unit::ByteIndex override {
            if (!positionable) {
                throw el::stream::StreamError{el::stream::StreamErrorContext{
                    "Failed to set the test stream position."_el, "The native test stream is not positionable."_el}};
            }
            streamPosition = position.toSizeTOrThrow();
            return position;
        }
        auto movePosition(const el::stream::StreamPositionOrigin origin, const el::unit::ByteOffset offset)
            -> el::unit::ByteIndex override {
            auto base = std::size_t{0U};
            if (origin == el::stream::StreamPositionOrigin::Current) {
                base = streamPosition;
            } else if (origin == el::stream::StreamPositionOrigin::End) {
                base = std::max(input.size(), output.size());
            }
            const auto target = el::unit::ByteIndex::fromSizeT(base).movedOrThrow(offset);
            return setPosition(target);
        }

        [[nodiscard]] auto read(const std::span<Byte> destination) -> ByteLength override {
            auto lock = std::unique_lock{mutex};
            readStarted.store(true);
            condition.notify_all();
            condition.wait(lock, [this] { return readAllowed || aborted.load(); });
            if (aborted.load()) {
                readFinished.store(true);
                if (failAfterAbort) {
                    throw el::stream::StreamError{el::stream::StreamErrorContext{
                        "Failed to read from the test stream."_el, "The native test read failed after aborting."_el}};
                }
                return ByteLength::zero();
            }
            if (streamPosition >= input.size()) {
                readFinished.store(true);
                return ByteLength::zero();
            }
            const auto count = std::min(destination.size(), input.size() - streamPosition);
            std::copy_n(input.data() + streamPosition, count, destination.data());
            streamPosition += count;
            readFinished.store(true);
            return ByteLength::fromSizeT(count);
        }

        void write(const std::span<const Byte> bytes) override {
            auto lock = std::unique_lock{mutex};
            writeStarted.store(true);
            condition.notify_all();
            condition.wait(lock, [this] { return writeAllowed || aborted.load(); });
            if (aborted.load()) {
                writeFinished.store(true);
                if (failAfterAbort) {
                    throw el::stream::StreamError{el::stream::StreamErrorContext{
                        "Failed to write to the test stream."_el, "The native test write failed after aborting."_el}};
                }
                return;
            }
            if (output.size() < streamPosition + bytes.size()) {
                output.resize(streamPosition + bytes.size());
            }
            std::copy(bytes.begin(), bytes.end(), output.begin() + static_cast<std::ptrdiff_t>(streamPosition));
            streamPosition += bytes.size();
            writeFinished.store(true);
        }

        void flush() override {}
        void close() override { closed.store(true); }
        void abort() noexcept override {
            {
                const auto lock = std::scoped_lock{mutex};
                aborted.store(true);
            }
            condition.notify_all();
        }

        void allowReads() {
            const auto lock = std::scoped_lock{mutex};
            readAllowed = true;
            condition.notify_all();
        }

        void allowWrites() {
            const auto lock = std::scoped_lock{mutex};
            writeAllowed = true;
            condition.notify_all();
        }

    public:
        std::mutex mutex;
        std::condition_variable condition;
        std::vector<Byte> input;
        std::vector<Byte> output;
        std::size_t streamPosition{0U};
        bool positionable{false};
        bool readAllowed{false};
        bool writeAllowed{false};
        bool failAfterAbort{false};
        std::atomic<bool> aborted{false};
        std::atomic<bool> closed{false};
        std::atomic<bool> readStarted{false};
        std::atomic<bool> readFinished{false};
        std::atomic<bool> writeStarted{false};
        std::atomic<bool> writeFinished{false};
    };

    [[nodiscard]] static auto inputSettings() -> el::stream::InputStreamSettings {
        return el::stream::InputStreamSettings{}
            .setTimeout(el::time::TimeDelta::milliseconds(20))
            .setBufferCapacity(ByteLength{4U});
    }

    [[nodiscard]] static auto outputSettings() -> el::stream::OutputStreamSettings {
        return el::stream::OutputStreamSettings{}
            .setTimeout(el::time::TimeDelta::milliseconds(20))
            .setBufferCapacity(ByteLength{4U})
            .setBackBufferLimit(ByteLength{8U});
    }

    void requireEventually(const std::atomic<bool> &value) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds{500};
        while (!value.load() && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::yield();
        }
        REQUIRE(value.load());
    }

    template <typename T>
    void requireRemainsClosed(const T &stream) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds{50};
        while (std::chrono::steady_clock::now() < deadline) {
            REQUIRE(stream.state() == el::stream::StreamState::Closed);
            std::this_thread::yield();
        }
    }

public:
    void testDataTypesAndIoServiceAreAccessible() {
        const auto native = std::make_shared<NativeStream>();
        const auto inputData = std::make_shared<el::stream::impl::BufferedByteInputStreamData>(native, inputSettings());
        const auto outputData =
            std::make_shared<el::stream::impl::BufferedByteOutputStreamData>(native, outputSettings());

        REQUIRE_EQUAL(inputData->native, native);
        REQUIRE_EQUAL(inputData->front.capacity(), ByteLength{4U});
        REQUIRE_EQUAL(outputData->native, native);
        REQUIRE_EQUAL(outputData->front.capacity(), ByteLength{4U});
        REQUIRE_EQUAL(&el::stream::impl::IoService::service(), &el::stream::impl::IoService::service());
        REQUIRE_EQUAL(el::stream::impl::IoService::cMaximumWorkerCount, 128U);

        const auto promise = std::make_shared<std::promise<void>>();
        auto future = promise->get_future();
        el::stream::impl::IoService::submitIoWork([promise] { promise->set_value(); });
        REQUIRE_EQUAL(future.wait_for(std::chrono::milliseconds{500}), std::future_status::ready);
    }

    void testInputTimeoutAndData() {
        const auto native = std::make_shared<NativeStream>();
        native->input = {Byte{1U}, Byte{2U}, Byte{3U}};
        auto stream = el::stream::impl::BufferedByteInputStream{native, inputSettings()};
        auto bytes = std::array<Byte, 4>{};

        REQUIRE(stream.read(bytes) == StreamReadStatus::Timeout);
        native->allowReads();
        REQUIRE(stream.waitForReady() == StreamWaitStatus::Ready);
        const auto result = stream.read(bytes);
        REQUIRE(result == StreamReadStatus::Data);
        REQUIRE_EQUAL(result.data(), ByteLength{3U});
        REQUIRE_EQUAL(bytes[0], Byte{1U});
    }

    void testAtomicBackBufferAndCloseTimeout() {
        const auto native = std::make_shared<NativeStream>();
        auto stream = el::stream::impl::BufferedByteOutputStream{native, outputSettings()};
        const auto front = std::array{Byte{1U}, Byte{2U}, Byte{3U}, Byte{4U}};
        const auto back = std::array{Byte{5U}, Byte{6U}, Byte{7U}, Byte{8U}, Byte{9U}, Byte{10U}, Byte{11U}, Byte{12U}};

        REQUIRE(stream.write(front) == StreamWriteStatus::Success);
        REQUIRE(stream.write(back) == StreamWriteStatus::Success);
        REQUIRE_FALSE(stream.isReady());
        REQUIRE(stream.write(Byte{13U}) == StreamWriteStatus::Timeout);
        REQUIRE(stream.close() == StreamCloseStatus::Timeout);

        native->allowWrites();
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds{500};
        while (stream.state() != el::stream::StreamState::Closed && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::yield();
        }
        REQUIRE(stream.close() == StreamCloseStatus::Closed);
        REQUIRE_EQUAL(native->output.size(), std::size_t{12U});
    }

    void testTimedOutWriteCanBeRetriedWithoutDuplication() {
        const auto native = std::make_shared<NativeStream>();
        auto stream = el::stream::impl::BufferedByteOutputStream{native, outputSettings()};
        const auto front = std::array{Byte{1U}, Byte{2U}, Byte{3U}, Byte{4U}};
        const auto back = std::array{Byte{5U}, Byte{6U}, Byte{7U}, Byte{8U}, Byte{9U}, Byte{10U}, Byte{11U}, Byte{12U}};

        REQUIRE(stream.write(front).isSuccess());
        REQUIRE(stream.write(back).isSuccess());
        REQUIRE(stream.write(Byte{13U}).isTimeout());

        native->allowWrites();
        REQUIRE(stream.waitForReady().isReady());
        REQUIRE(stream.write(Byte{13U}).isSuccess());
        REQUIRE(stream.close().isClosed());
        REQUIRE_EQUAL(native->output.size(), std::size_t{13U});
        REQUIRE_EQUAL(native->output.back(), Byte{13U});
    }

    void testBufferedEncodedWritesBomOnlyOnce() {
        const auto native = std::make_shared<NativeStream>();
        native->allowWrites();
        const auto byteStream = std::make_shared<el::stream::impl::BufferedByteOutputStream>(native, outputSettings());
        auto stream = el::stream::impl::EncodedTextOutputStream{
            byteStream, el::text::StringEncoding::Utf16, el::text::StringBomMode::Require};

        REQUIRE(stream.write("A"_el).isSuccess());
        REQUIRE(stream.write(el::text::Char{U'B'}).isSuccess());
        REQUIRE(stream.writeLine("C"_el).isSuccess());
        REQUIRE(stream.close().isClosed());

        REQUIRE_EQUAL(
            native->output,
            std::vector<Byte>(
                {Byte{0xffU},
                    Byte{0xfeU},
                    Byte{0x41U},
                    Byte{0x00U},
                    Byte{0x42U},
                    Byte{0x00U},
                    Byte{0x43U},
                    Byte{0x00U},
                    Byte{0x0aU},
                    Byte{0x00U}}));
    }

    void testInputPositionIgnoresReadAheadAndResetsBuffers() {
        const auto native = std::make_shared<NativeStream>();
        native->positionable = true;
        native->readAllowed = true;
        native->input = {Byte{1U}, Byte{2U}, Byte{3U}, Byte{4U}, Byte{5U}};
        auto stream = el::stream::impl::BufferedByteInputStream{native, inputSettings()};
        auto bytes = std::array<Byte, 2>{};

        REQUIRE(stream.waitForReady().isReady());
        REQUIRE_EQUAL(stream.position(), el::unit::ByteIndex{0U});
        REQUIRE_EQUAL(stream.read(bytes).data(), ByteLength{2U});
        REQUIRE_EQUAL(stream.position(), el::unit::ByteIndex{2U});
        REQUIRE(stream.movePosition(StreamPositionOrigin::Current, ByteOffset{-1}).isSuccess());
        REQUIRE_EQUAL(stream.position(), el::unit::ByteIndex{1U});
        REQUIRE(stream.waitForReady().isReady());
        REQUIRE_EQUAL(stream.readByte().data(), Byte{2U});
    }

    void testOutputPositionWaitsForAcceptedWrites() {
        const auto native = std::make_shared<NativeStream>();
        native->positionable = true;
        auto stream = el::stream::impl::BufferedByteOutputStream{native, outputSettings()};
        const auto bytes = std::array{Byte{1U}, Byte{2U}, Byte{3U}};

        REQUIRE(stream.write(bytes).isSuccess());
        REQUIRE_EQUAL(stream.position(), el::unit::ByteIndex{3U});
        REQUIRE(stream.setPosition(el::unit::ByteIndex{1U}).isTimeout());
        REQUIRE_EQUAL(stream.position(), el::unit::ByteIndex{3U});

        native->allowWrites();
        REQUIRE(stream.setPosition(el::unit::ByteIndex{1U}).isSuccess());
        REQUIRE(stream.write(Byte{9U}).isSuccess());
        REQUIRE(stream.close().isClosed());
        REQUIRE_EQUAL(native->output, std::vector<Byte>({Byte{1U}, Byte{9U}, Byte{3U}}));
    }

    void testHardLimitThrowsWithoutPartialWrite() {
        const auto native = std::make_shared<NativeStream>();
        auto stream = el::stream::impl::BufferedByteOutputStream{native, outputSettings()};
        const auto tooLarge = std::array<Byte, 9>{};

        REQUIRE_THROWS_AS(el::stream::StreamError, stream.write(tooLarge));
        native->allowWrites();
        REQUIRE(stream.close() == StreamCloseStatus::Closed);
        REQUIRE(native->output.empty());
    }

    void testDestructionAbortsWithoutWaiting() {
        const auto native = std::make_shared<NativeStream>();
        const auto start = std::chrono::steady_clock::now();
        {
            auto stream = std::make_unique<el::stream::impl::BufferedByteOutputStream>(native, outputSettings());
            stream->write(Byte{1U});
        }
        const auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);

        REQUIRE(native->aborted.load());
        REQUIRE_LESS(elapsed, std::chrono::milliseconds{200});
    }

    void testInputAbortKeepsClosedStateWhenNativeReadFails() {
        const auto native = std::make_shared<NativeStream>();
        native->failAfterAbort = true;
        auto stream = el::stream::impl::BufferedByteInputStream{native, inputSettings()};

        WITH_CONTEXT(requireEventually(native->readStarted));
        stream.abort();
        WITH_CONTEXT(requireEventually(native->readFinished));

        WITH_CONTEXT(requireRemainsClosed(stream));
    }

    void testOutputAbortKeepsClosedStateWhenNativeWriteFails() {
        const auto native = std::make_shared<NativeStream>();
        native->failAfterAbort = true;
        auto stream = el::stream::impl::BufferedByteOutputStream{native, outputSettings()};

        REQUIRE(stream.write(Byte{1U}).isSuccess());
        WITH_CONTEXT(requireEventually(native->writeStarted));
        stream.abort();
        WITH_CONTEXT(requireEventually(native->writeFinished));

        WITH_CONTEXT(requireRemainsClosed(stream));
    }
};
