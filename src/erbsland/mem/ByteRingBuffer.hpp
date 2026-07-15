// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Endianness.hpp"
#include "RingBuffer.hpp"

#include <array>
#include <concepts>
#include <cstdint>
#include <optional>
#include <type_traits>

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
        auto bytes = std::array<Byte, sizeof(T)>{};
        static_cast<void>(read(std::span<Byte>{bytes}));
        auto result = std::make_unsigned_t<T>{0};
        for (auto i = std::size_t{0}; i < bytes.size(); ++i) {
            const auto shift = _endianness == Endianness::Little ? i * 8U : (bytes.size() - i - 1U) * 8U;
            result |= static_cast<std::make_unsigned_t<T>>(bytes[i].toUInt8()) << shift;
        }
        return static_cast<T>(result);
    }
    /// Atomically write an integer.
    /// @return `Failure` without modification if the hard storage limit would be exceeded.
    template <std::integral T>
    [[nodiscard]] auto writeInteger(const T value) -> util::Result {
        using Unsigned = std::make_unsigned_t<T>;
        const auto unsignedValue = static_cast<Unsigned>(value);
        auto bytes = std::array<Byte, sizeof(T)>{};
        for (auto i = std::size_t{0}; i < bytes.size(); ++i) {
            const auto shift = _endianness == Endianness::Little ? i * 8U : (bytes.size() - i - 1U) * 8U;
            bytes[i] = Byte{static_cast<uint8_t>((unsignedValue >> shift) & Unsigned{0xffU})};
        }
        return writeExact(std::span<const Byte>{bytes});
    }

private:
    Endianness _endianness{Endianness::Little};
};

}
