// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ByteInputStream_fwd.hpp"
#include "InputStream.hpp"
#include "StreamReadResult.hpp"

#include "impl/EncodedTextInputStream_fwd.hpp"

#include "../mem/Byte.hpp"
#include "../mem/ByteBlock.hpp"
#include "../mem/ByteSpan.hpp"
#include "../mem/Endianness.hpp"
#include "../mem/RingBuffer.hpp"
#include "../time/TimePoint.hpp"
#include "../unit/ByteIndex.hpp"
#include "../unit/ByteLength.hpp"
#include "../util/CoAsyncGenerator.hpp"
#include "../util/CoTask.hpp"

#include <concepts>
#include <cstdint>
#include <mutex>
#include <optional>
#include <span>
#include <type_traits>

namespace erbsland::stream {

/// A stream that reads raw bytes.
/// Chunk reads return currently available data and may be short. Logical reads retain partial input internally across
/// timeouts, so exact blocks and integer values never require caller-side reassembly.
/// @tested{ByteStreamTest AsyncStreamTest}
class ByteInputStream : public InputStream {
    friend class impl::EncodedTextInputStream;

protected:
    /// The absolute deadline shared by all source reads for one public operation.
    using ReadDeadline = time::TimePoint;

public:
    /// The default maximum for no-argument aggregate reads: 10 MiB.
    static constexpr auto cDefaultByteReadMaximum = unit::ByteLength{10U * 1024U * 1024U};

public:
    // defaults
    ~ByteInputStream() override = default;

public: // accessors
    /// Get the byte order used by integer convenience methods.
    [[nodiscard]] auto endianness() const noexcept -> mem::Endianness;
    /// Set the byte order used by integer convenience methods.
    void setEndianness(mem::Endianness endianness) noexcept;

public: // implement StreamPositioning
    [[nodiscard]] auto supportsPositioning() const noexcept -> bool override;
    [[nodiscard]] auto position() const -> unit::ByteIndex override;
    auto setPosition(unit::ByteIndex position) -> StreamPositionStatus override;
    auto movePosition(StreamPositionOrigin origin, unit::ByteOffset offset) -> StreamPositionStatus override;

public: // core interface
    /// Read bytes into a destination span.
    /// @param destination The destination bytes to fill.
    /// @return The status and number of bytes read.
    /// @throws stream::StreamError If the stream is closed or the backing source fails.
    [[nodiscard]] auto read(mem::ByteSpan destination) -> StreamReadResult<unit::ByteLength>;

public: // default interface
    /// Read up to `maximumLength` bytes and return them as a byte block.
    /// @param maximumLength The maximum number of bytes to read.
    /// @return The status and bytes that were read.
    /// @throws err::ParameterError If `maximumLength` is infinite.
    /// @throws stream::StreamError If the stream is closed or the backing source fails.
    [[nodiscard]] auto read(unit::ByteLength maximumLength) -> StreamReadResult<mem::ByteBlock>;
    /// Read exactly `length` bytes.
    /// @param length The number of bytes to read.
    /// Partial bytes are retained by the stream after timeout or premature end-of-stream. Repeating this operation
    /// continues it; selecting another read operation makes retained bytes available to that operation in order.
    /// @return `Data` with exactly `length` bytes, or an empty `Timeout`/`Finished` result.
    /// @throws err::ParameterError If `length` is infinite.
    /// @throws stream::StreamError If the stream is closed or the backing source fails.
    [[nodiscard]] auto readExact(unit::ByteLength length) -> StreamReadResult<mem::ByteBlock>;
    /// Read one byte.
    /// @return The status and next byte.
    /// @throws stream::StreamError If the stream is closed or the backing source fails.
    [[nodiscard]] auto readByte() -> StreamReadResult<mem::Byte>;
    /// Read remaining bytes up to `cDefaultByteReadMaximum`.
    /// @return The status and bytes read before the limit, end, or timeout.
    /// @throws stream::StreamError If the stream is closed or the backing source fails.
    [[nodiscard]] auto readAll() -> StreamReadResult<mem::ByteBlock>;
    /// Read remaining bytes up to `maximumLength`.
    /// @param maximumLength The maximum number of bytes to aggregate.
    /// @return The status and bytes read before the limit, end, or timeout.
    /// @throws err::ParameterError If `maximumLength` is infinite.
    /// @throws stream::StreamError If the stream is closed or the backing source fails.
    [[nodiscard]] auto readAll(unit::ByteLength maximumLength) -> StreamReadResult<mem::ByteBlock>;

public: // coroutine interface
    /// Asynchronously read one owned byte block.
    /// @param maximumLength The maximum number of bytes to read.
    /// @return A task with the same result as `read(maximumLength)`.
    /// @throws err::LogicError If this stream is not shared-owned.
    /// @throws err::ParameterError When the task result is observed if `maximumLength` is infinite.
    /// @throws stream::StreamError When the task result is observed if the stream or backing source fails.
    [[nodiscard]] auto coRead(unit::ByteLength maximumLength) -> util::CoTask<StreamReadResult<mem::ByteBlock>>;
    /// Asynchronously read exactly `length` bytes.
    /// @param length The number of bytes to read.
    /// @return A task with the same result as `readExact(length)`.
    /// @throws err::LogicError If this stream is not shared-owned.
    /// @throws err::ParameterError When the task result is observed if `length` is infinite.
    /// @throws stream::StreamError When the task result is observed if the stream or backing source fails.
    [[nodiscard]] auto coReadExact(unit::ByteLength length) -> util::CoTask<StreamReadResult<mem::ByteBlock>>;
    /// Asynchronously read remaining bytes up to `cDefaultByteReadMaximum`.
    /// @return A task with the same result as `readAll()`.
    /// @throws err::LogicError If this stream is not shared-owned.
    /// @throws stream::StreamError When the task result is observed if the stream or backing source fails.
    [[nodiscard]] auto coReadAll() -> util::CoTask<StreamReadResult<mem::ByteBlock>>;
    /// Asynchronously read remaining bytes up to `maximumLength`.
    /// @param maximumLength The maximum aggregate length.
    /// @return A task with the same result as `readAll(maximumLength)`.
    /// @throws err::LogicError If this stream is not shared-owned.
    /// @throws err::ParameterError When the task result is observed if `maximumLength` is infinite.
    /// @throws stream::StreamError When the task result is observed if the stream or backing source fails.
    [[nodiscard]] auto coReadAll(unit::ByteLength maximumLength) -> util::CoTask<StreamReadResult<mem::ByteBlock>>;
    /// Asynchronously generate byte blocks using the default maximum block length.
    /// Timeout results are yielded; end-of-stream completes the generator.
    /// @return A lazy generator for bounded byte-block results.
    /// @throws err::LogicError When first advanced if this stream is not shared-owned.
    /// @throws stream::StreamError When advancing if the stream or backing source fails.
    [[nodiscard]] auto coReadBlocks() -> util::CoAsyncGenerator<StreamReadResult<mem::ByteBlock>>;
    /// Asynchronously generate byte blocks.
    /// @param maximumLength The positive finite maximum for each block.
    /// Timeout results are yielded; end-of-stream completes the generator.
    /// @return A lazy generator for bounded byte-block results.
    /// @throws err::ParameterError When first advanced if `maximumLength` is zero or infinite.
    /// @throws err::LogicError When first advanced if this stream is not shared-owned.
    /// @throws stream::StreamError When advancing if the stream or backing source fails.
    [[nodiscard]] auto coReadBlocks(unit::ByteLength maximumLength)
        -> util::CoAsyncGenerator<StreamReadResult<mem::ByteBlock>>;

public: // integer read
    /// Read an integer value.
    /// @return The status and integer value.
    /// @throws stream::StreamError If the stream is closed or the backing source fails.
    template <std::integral T>
    [[nodiscard]] auto readInteger() -> StreamReadResult<T> {
        const auto data = readExact(unit::ByteLength::fromSizeT(sizeof(T)));
        if (data != StreamReadStatus::Data) {
            return {data, T{}};
        }
        auto result = std::make_unsigned_t<T>{0};
        for (auto i = std::size_t{0}; i < sizeof(T); ++i) {
            const auto byte =
                static_cast<std::make_unsigned_t<T>>(data.data().get(unit::ByteIndex::fromSizeT(i)).toUInt8());
            const auto shift = _endianness == mem::Endianness::Little ? i * 8U : (sizeof(T) - 1U - i) * 8U;
            result |= byte << shift;
        }
        return {StreamReadStatus::Data, static_cast<T>(result)};
    }

public: // integer wrappers
    /// Read a signed 8-bit integer.
    [[nodiscard]] auto readInt8() -> StreamReadResult<int8_t> { return readInteger<int8_t>(); }
    /// Read an unsigned 8-bit integer.
    [[nodiscard]] auto readUInt8() -> StreamReadResult<uint8_t> { return readInteger<uint8_t>(); }
    /// Read a signed 16-bit integer.
    [[nodiscard]] auto readInt16() -> StreamReadResult<int16_t> { return readInteger<int16_t>(); }
    /// Read an unsigned 16-bit integer.
    [[nodiscard]] auto readUInt16() -> StreamReadResult<uint16_t> { return readInteger<uint16_t>(); }
    /// Read a signed 32-bit integer.
    [[nodiscard]] auto readInt32() -> StreamReadResult<int32_t> { return readInteger<int32_t>(); }
    /// Read an unsigned 32-bit integer.
    [[nodiscard]] auto readUInt32() -> StreamReadResult<uint32_t> { return readInteger<uint32_t>(); }
    /// Read a signed 64-bit integer.
    [[nodiscard]] auto readInt64() -> StreamReadResult<int64_t> { return readInteger<int64_t>(); }
    /// Read an unsigned 64-bit integer.
    [[nodiscard]] auto readUInt64() -> StreamReadResult<uint64_t> { return readInteger<uint64_t>(); }

protected:
    /// Create the absolute deadline for a public read operation.
    [[nodiscard]] auto deadlineFromNow() const -> ReadDeadline;

protected:
    /// Read until the destination is full or the deadline is reached.
    [[nodiscard]] auto readUntil(mem::ByteSpan destination, ReadDeadline deadline)
        -> StreamReadResult<unit::ByteLength>;

private:
    /// Read one source chunk while the logical-read lock is held.
    [[nodiscard]] auto readChunkLocked(mem::ByteSpan destination, ReadDeadline deadline)
        -> StreamReadResult<unit::ByteLength>;
    /// Read source bytes into retained storage.
    [[nodiscard]] auto readIntoRetained(unit::ByteLength maximumLength, ReadDeadline deadline) -> StreamReadStatus;
    /// Prepare a bounded read from retained and source bytes.
    [[nodiscard]] auto prepareRead(unit::ByteLength maximumLength, ReadDeadline deadline) -> StreamReadStatus;
    /// Prepare an exact-length read from retained and source bytes.
    [[nodiscard]] auto prepareExact(unit::ByteLength length, ReadDeadline deadline) -> StreamReadStatus;
    /// Prepare an aggregate read from retained and source bytes.
    [[nodiscard]] auto prepareAll(unit::ByteLength maximumLength, ReadDeadline deadline) -> StreamReadStatus;
    /// Get the retained-input buffer, creating it when necessary.
    [[nodiscard]] auto retainedBuffer() -> mem::RingBuffer &;
    /// Take a byte prefix from retained input.
    [[nodiscard]] auto takeRetained(unit::ByteLength length) -> mem::ByteBlock;

protected:
    /// Read the next source chunk using the remaining time before `deadline`.
    [[nodiscard]] virtual auto readFromSource(mem::ByteSpan destination, ReadDeadline deadline)
        -> StreamReadResult<unit::ByteLength> = 0;
    /// Test if retained input can be returned without accessing the source.
    [[nodiscard]] auto hasRetainedInput() const noexcept -> bool;
    /// Discard pending and replay input during close or abort.
    void discardRetainedInput() noexcept;
    /// Test if the underlying byte source supports positioning.
    [[nodiscard]] virtual auto sourceSupportsPositioning() const noexcept -> bool;
    /// Get the logical position of the underlying byte source.
    [[nodiscard]] virtual auto sourcePosition() const -> unit::ByteIndex;
    /// Set the underlying byte-source position.
    virtual auto setSourcePosition(unit::ByteIndex position) -> StreamPositionStatus;
    /// Move the underlying byte-source position.
    virtual auto moveSourcePosition(StreamPositionOrigin origin, unit::ByteOffset offset) -> StreamPositionStatus;

private:
    /// Clear retained input without replaying it.
    void clearRetainedInput() noexcept;

private:
    mem::Endianness _endianness{mem::Endianness::Little}; ///< Integer byte order.
    mutable std::mutex _readMutex;                        ///< Serializes logical read state.
    std::optional<mem::RingBuffer> _retainedBytes;        ///< Ordered bytes retained across logical operations.
};

}
