// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ByteArray.hpp"
#include "ByteSpan.hpp"
#include "Endianness.hpp"
#include "RingBuffer.hpp"

#include "impl/UnsafeByteArrayAccess.hpp"

#include <concepts>
#include <optional>

namespace erbsland::mem {

/// A byte ring with atomic endian-aware integer operations.
/// @tested{RingBufferTest}
class ByteRingBuffer final : public RingBuffer {
public:
    using RingBuffer::RingBuffer;

public: // accessors
    /// Get the byte order used by integer operations.
    [[nodiscard]] auto endianness() const noexcept -> Endianness { return _endianness; }
    /// Set the byte order used by integer operations.
    void setEndianness(const Endianness value) noexcept { _endianness = value; }

public: // integers
    /// Atomically read an integer.
    /// @return The value, or an empty optional if there are not enough readable bytes.
    template <std::integral T>
    [[nodiscard]] auto readInteger() -> std::optional<T> {
        if (length().toSizeT() < sizeof(T)) {
            return std::nullopt;
        }
        auto bytes = ByteArray<sizeof(T)>{};
        static_cast<void>(read(impl::UnsafeByteArrayAccess{bytes}.writableData()));
        return bytes.template getInteger<T>(unit::ByteIndex::zero(), _endianness);
    }
    /// Atomically write an integer.
    /// @return `Failure` without modification if the hard storage limit would be exceeded.
    template <std::integral T>
    [[nodiscard]] auto writeInteger(const T value) -> util::Result {
        auto bytes = ByteArray<sizeof(T)>{};
        bytes.setIntegerOrThrow(unit::ByteIndex::zero(), value, _endianness);
        return writeExact(bytes.span());
    }

private:
    Endianness _endianness{Endianness::Little};
};

}
