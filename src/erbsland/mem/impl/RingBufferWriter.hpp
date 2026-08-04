// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UnsafeRingBufferAccess.hpp"

#include "../ByteSpan.hpp"
#include "../Endianness.hpp"

#include "../../err/LogicError.hpp"
#include "../../text/Literals.hpp"
#include "../../unit/ByteIndex.hpp"
#include "../../unit/ByteLength.hpp"

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

namespace erbsland::mem::impl {

using namespace text::literals;

/// A transactional sequential writer over the currently writable spans of a ring buffer.
/// The caller must reserve enough space before construction. Written bytes remain invisible until `commit()`.
/// @tested{StringEncoderTest}
class RingBufferWriter final {
public:
    /// Start a transactional write into the available storage of a ring buffer.
    explicit RingBufferWriter(RingBuffer &buffer) : _access{buffer}, _spans{_access.writableSpans()} {}

    // defaults/deletions
    ~RingBufferWriter() = default;
    RingBufferWriter(const RingBufferWriter &) = delete;
    RingBufferWriter(RingBufferWriter &&) = delete;
    auto operator=(const RingBufferWriter &) -> RingBufferWriter & = delete;
    auto operator=(RingBufferWriter &&) -> RingBufferWriter & = delete;

public: // accessors
    /// Get the number of bytes written in this transaction.
    [[nodiscard]] auto position() const noexcept -> unit::ByteIndex { return unit::ByteIndex::fromSizeT(_position); }
    /// Get the byte order for multi-byte integers.
    [[nodiscard]] auto endianness() const noexcept -> Endianness { return _endianness; }
    /// Set the byte order for multi-byte integers.
    void setEndianness(const Endianness endianness) noexcept { _endianness = endianness; }

public: // write
    /// Write a byte into the reserved ring storage.
    auto writeByte(const Byte value) -> RingBufferWriter & {
        while (_spanIndex < _spans.size() && _spanPosition >= _spans[_spanIndex].size()) {
            ++_spanIndex;
            _spanPosition = 0U;
        }
        if (_spanIndex >= _spans.size()) {
            throw err::LogicError{"Ring buffer writer exceeded the reserved storage."_el};
        }
        _spans[_spanIndex][_spanPosition] = value;
        ++_spanPosition;
        ++_position;
        return *this;
    }
    /// Write an integer into the reserved ring storage.
    template <std::integral T>
    auto writeInteger(const T value) -> RingBufferWriter & {
        using Unsigned = std::make_unsigned_t<T>;
        const auto unsignedValue = static_cast<Unsigned>(value);
        for (auto i = std::size_t{0}; i < sizeof(T); ++i) {
            const auto shift = _endianness == Endianness::Little ? i * 8U : (sizeof(T) - 1U - i) * 8U;
            writeByte(Byte{static_cast<uint8_t>((unsignedValue >> shift) & Unsigned{0xffU})});
        }
        return *this;
    }
    /// Write an unsigned 8-bit integer.
    auto writeUInt8(const uint8_t value) -> RingBufferWriter & { return writeInteger(value); }
    /// Write an unsigned 16-bit integer.
    auto writeUInt16(const uint16_t value) -> RingBufferWriter & { return writeInteger(value); }
    /// Write an unsigned 32-bit integer.
    auto writeUInt32(const uint32_t value) -> RingBufferWriter & { return writeInteger(value); }
    /// Commit all bytes written in this transaction.
    void commit() {
        if (_committed) {
            throw err::LogicError{"Ring buffer writer transaction was already committed."_el};
        }
        _access.commitWritten(unit::ByteLength::fromSizeT(_position));
        _committed = true;
    }

private:
    UnsafeRingBufferAccess _access;
    std::array<ByteSpan, 2> _spans;
    Endianness _endianness{Endianness::Little};
    std::size_t _spanIndex{0U};
    std::size_t _spanPosition{0U};
    std::size_t _position{0U};
    bool _committed{false};
};

}
