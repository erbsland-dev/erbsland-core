// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ByteOutputStream_fwd.hpp"
#include "OutputStream.hpp"

#include "../mem/Byte.hpp"
#include "../mem/ByteBlock.hpp"
#include "../mem/Endianness.hpp"
#include "../util/CoTask.hpp"

#include <array>
#include <concepts>
#include <cstdint>
#include <span>
#include <type_traits>

namespace erbsland::stream {

/// A stream that writes raw bytes.
/// Every write atomically accepts the complete request for asynchronous delivery. `Timeout` means no byte from that
/// call was accepted, so the complete unchanged call can be retried.
/// @tested{ByteStreamTest AsyncStreamTest}
class ByteOutputStream : public OutputStream {
public:
    ~ByteOutputStream() override = default;

public: // accessors
    /// Get the byte order used by integer convenience methods.
    [[nodiscard]] virtual auto endianness() const noexcept -> mem::Endianness;
    /// Set the byte order used by integer convenience methods.
    virtual void setEndianness(mem::Endianness endianness) noexcept;

public: // default interface
    /// Write a single byte.
    /// @param byte The byte to write.
    /// @return `Success` if the byte was accepted, or `Timeout` if nothing was accepted.
    /// @throws stream::StreamError If the stream is closed or the backing target fails.
    virtual auto write(mem::Byte byte) -> StreamWriteStatus;

public: // core interface
    /// Write a byte span.
    /// @param bytes The bytes to write.
    /// @return `Success` if all bytes were accepted, or `Timeout` if nothing was accepted.
    /// @throws stream::StreamError If the stream is closed or the backing target fails.
    virtual auto write(std::span<const mem::Byte> bytes) -> StreamWriteStatus = 0;

public: // default interface
    /// Write a read-only byte block.
    /// @param bytes The bytes to write.
    /// @return `Success` if all bytes were accepted, or `Timeout` if nothing was accepted.
    /// @throws stream::StreamError If the stream is closed or the backing target fails.
    virtual auto write(const mem::ByteBlock &bytes) -> StreamWriteStatus;

public: // coroutine interface
    /// Asynchronously write an owned byte block.
    /// @param bytes The bytes retained by the operation until it completes.
    /// @return A task with the same result as `write(ByteBlock)`.
    /// @throws err::LogicError If this stream is not shared-owned.
    /// @throws stream::StreamError When the task result is observed if the stream or backing target fails.
    [[nodiscard]] auto coWrite(mem::ByteBlock bytes) -> util::CoTask<StreamWriteStatus>;

public: // integer write
    /// Write an integer value.
    /// @param value The integer value.
    /// @return `Success` if the complete integer was accepted, or `Timeout` if nothing was accepted.
    /// @throws stream::StreamError If the stream is closed or the backing target fails.
    template <std::integral T>
    auto writeInteger(T value) -> StreamWriteStatus {
        using Unsigned = std::make_unsigned_t<T>;
        const auto unsignedValue = static_cast<Unsigned>(value);
        auto bytes = std::array<mem::Byte, sizeof(T)>{};
        for (auto i = std::size_t{0}; i < sizeof(T); ++i) {
            const auto shift = _endianness == mem::Endianness::Little ? i * 8U : (sizeof(T) - 1U - i) * 8U;
            bytes[i] = mem::Byte{static_cast<uint8_t>((unsignedValue >> shift) & Unsigned{0xffU})};
        }
        return write(std::span<const mem::Byte>{bytes});
    }

public: // integer wrappers
    /// Write a signed 8-bit integer.
    auto writeInt8(int8_t value) -> StreamWriteStatus { return writeInteger(value); }
    /// Write an unsigned 8-bit integer.
    auto writeUInt8(uint8_t value) -> StreamWriteStatus { return writeInteger(value); }
    /// Write a signed 16-bit integer.
    auto writeInt16(int16_t value) -> StreamWriteStatus { return writeInteger(value); }
    /// Write an unsigned 16-bit integer.
    auto writeUInt16(uint16_t value) -> StreamWriteStatus { return writeInteger(value); }
    /// Write a signed 32-bit integer.
    auto writeInt32(int32_t value) -> StreamWriteStatus { return writeInteger(value); }
    /// Write an unsigned 32-bit integer.
    auto writeUInt32(uint32_t value) -> StreamWriteStatus { return writeInteger(value); }
    /// Write a signed 64-bit integer.
    auto writeInt64(int64_t value) -> StreamWriteStatus { return writeInteger(value); }
    /// Write an unsigned 64-bit integer.
    auto writeUInt64(uint64_t value) -> StreamWriteStatus { return writeInteger(value); }

private:
    mem::Endianness _endianness{mem::Endianness::Little}; ///< Integer byte order.
};

}
