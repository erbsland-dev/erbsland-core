// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <bit>
#include <cstddef>
#include <cstdint>

namespace erbsland::mem {

/// A small wrapper around a single byte.
/// Provides a convenient interface for working with individual bytes and bits.
/// Shift operations return zero when the shift is eight or greater.
/// Rotation amounts are reduced modulo eight; negative amounts rotate in the opposite direction.
/// @tested{ByteTest}
class Byte final {
public:
    /// The raw byte value type.
    using Value = std::byte;

public:
    /// Create a byte from a raw byte value.
    constexpr Byte(const Value byte) : _byte{byte} {} // NOLINT(*-explicit-constructor)
    /// Create a byte from a uint8_t value.
    constexpr Byte(const uint8_t byte) : _byte{std::byte{byte}} {} // NOLINT(*-explicit-constructor)

    // defaults
    Byte() = default;
    ~Byte() = default;
    Byte(const Byte &) = default;
    auto operator=(const Byte &) -> Byte & = default;
    Byte(Byte &&) = default;
    auto operator=(Byte &&) -> Byte & = default;

public: // comparison
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_byte, const Byte &other, other._byte);

public: // operators
    /// Compute the bitwise OR of two bytes.
    [[nodiscard]] constexpr friend auto operator|(const Byte &lhs, const Byte &rhs) noexcept -> Byte {
        return {lhs._byte | rhs._byte};
    }
    /// Compute the bitwise AND of two bytes.
    [[nodiscard]] constexpr friend auto operator&(const Byte &lhs, const Byte &rhs) noexcept -> Byte {
        return {lhs._byte & rhs._byte};
    }
    /// Compute the bitwise XOR of two bytes.
    [[nodiscard]] constexpr friend auto operator^(const Byte &lhs, const Byte &rhs) noexcept -> Byte {
        return {lhs._byte ^ rhs._byte};
    }
    /// Apply a bitwise OR to this byte.
    constexpr auto operator|=(const Byte &other) noexcept -> Byte & {
        _byte |= other._byte;
        return *this;
    }
    /// Apply a bitwise AND to this byte.
    constexpr auto operator&=(const Byte &other) noexcept -> Byte & {
        _byte &= other._byte;
        return *this;
    }
    /// Apply a bitwise XOR to this byte.
    constexpr auto operator^=(const Byte &other) noexcept -> Byte & {
        _byte ^= other._byte;
        return *this;
    }
    /// Invert every bit.
    constexpr auto operator~() const noexcept -> Byte { return {_byte ^ std::byte{0xFFU}}; }
    /// Shift bits left and fill with zero.
    [[nodiscard]] constexpr auto operator<<(const std::size_t shift) const noexcept -> Byte {
        return shiftedLeft(shift);
    }
    /// Shift bits right and fill with zero.
    [[nodiscard]] constexpr auto operator>>(const std::size_t shift) const noexcept -> Byte {
        return shiftedRight(shift);
    }
    /// Shift this byte left and fill with zero.
    constexpr auto operator<<=(const std::size_t shift) noexcept -> Byte & {
        shiftLeft(shift);
        return *this;
    }
    /// Shift this byte right and fill with zero.
    constexpr auto operator>>=(const std::size_t shift) noexcept -> Byte & {
        shiftRight(shift);
        return *this;
    }

public: // tools
    /// Return this byte shifted left.
    /// @param shift The number of bit positions.
    /// @return The shifted byte, or zero if `shift` is eight or greater.
    [[nodiscard]] constexpr auto shiftedLeft(const std::size_t shift) const noexcept -> Byte {
        if (shift >= 8U) {
            return {};
        }
        return Byte{static_cast<uint8_t>(toUInt8() << shift)};
    }
    /// Shift this byte left in place.
    /// @param shift The number of bit positions.
    constexpr void shiftLeft(const std::size_t shift) noexcept { *this = shiftedLeft(shift); }
    /// Return this byte shifted right.
    /// @param shift The number of bit positions.
    /// @return The shifted byte, or zero if `shift` is eight or greater.
    [[nodiscard]] constexpr auto shiftedRight(const std::size_t shift) const noexcept -> Byte {
        if (shift >= 8U) {
            return {};
        }
        return Byte{static_cast<uint8_t>(toUInt8() >> shift)};
    }
    /// Shift this byte right in place.
    /// @param shift The number of bit positions.
    constexpr void shiftRight(const std::size_t shift) noexcept { *this = shiftedRight(shift); }
    /// Return this byte rotated left.
    /// @param amount The signed rotation amount.
    /// @return The rotated byte.
    [[nodiscard]] constexpr auto rotatedLeft(const int amount) const noexcept -> Byte {
        return Byte{std::rotl(toUInt8(), amount)};
    }
    /// Rotate this byte left in place.
    /// @param amount The signed rotation amount.
    constexpr void rotateLeft(const int amount) noexcept { *this = rotatedLeft(amount); }
    /// Return this byte rotated right.
    /// @param amount The signed rotation amount.
    /// @return The rotated byte.
    [[nodiscard]] constexpr auto rotatedRight(const int amount) const noexcept -> Byte {
        return Byte{std::rotr(toUInt8(), amount)};
    }
    /// Rotate this byte right in place.
    /// @param amount The signed rotation amount.
    constexpr void rotateRight(const int amount) noexcept { *this = rotatedRight(amount); }
    /// Apply a bit mask to this byte.
    /// @param mask The bits to retain.
    /// @return The masked byte.
    [[nodiscard]] constexpr auto masked(const Byte mask) const noexcept -> Byte { return {_byte & mask._byte}; }
    /// Test if the masked bits equal the expected value.
    /// @param mask The bits to compare.
    /// @param expected The expected masked value.
    /// @return `true` if the masked bits match.
    [[nodiscard]] constexpr auto matches(const Byte mask, const Byte expected) const noexcept -> bool {
        return masked(mask) == expected;
    }

public: // conversion
    /// Get the byte as `std::byte` value.
    [[nodiscard]] constexpr auto toStdByte() const noexcept -> std::byte { return _byte; }
    /// Get the byte as `char` value.
    [[nodiscard]] constexpr auto toChar() const noexcept -> char { return static_cast<char>(_byte); }
    /// Get the byte as an unsigned integer.
    [[nodiscard]] constexpr auto toUInt8() const noexcept -> uint8_t { return static_cast<uint8_t>(_byte); }
    /// @overload
    [[nodiscard]] constexpr auto toUInt16() const noexcept -> uint16_t { return static_cast<uint16_t>(_byte); }
    /// @overload
    [[nodiscard]] constexpr auto toUInt32() const noexcept -> uint32_t { return static_cast<uint32_t>(_byte); }
    /// @overload
    [[nodiscard]] constexpr auto toUInt64() const noexcept -> uint64_t { return static_cast<uint64_t>(_byte); }
    /// Get the underlying raw value.
    /// Please use one of the conversion functions instead, as this leads to future proof and more readable code.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> std::byte { return _byte; }
    /// Create a byte from a char.
    /// @param value The character whose bit pattern is copied.
    /// @return The byte value.
    [[nodiscard]] static constexpr auto fromChar(const char value) noexcept -> Byte {
        return {static_cast<std::byte>(value)};
    }
    /// Create a byte from the lowest bits of an integer.
    /// @param value The unsigned integer value.
    /// @return The byte value.
    [[nodiscard]] static constexpr auto fromUInt8(const uint8_t value) noexcept -> Byte {
        return {static_cast<std::byte>(value)};
    }
    /// @overload
    [[nodiscard]] static constexpr auto fromCroppedUInt16(const uint16_t value) noexcept -> Byte {
        return {static_cast<std::byte>(value & 0xFFU)};
    }
    /// @overload
    [[nodiscard]] static constexpr auto fromCroppedUInt32(const uint32_t value) noexcept -> Byte {
        return {static_cast<std::byte>(value & 0xFFU)};
    }
    /// @overload
    [[nodiscard]] static constexpr auto fromCroppedUInt64(const uint64_t value) noexcept -> Byte {
        return {static_cast<std::byte>(value & 0xFFU)};
    }

private:
    Value _byte{0U}; ///< The actual byte value.
};

static_assert(sizeof(Byte) == sizeof(std::byte));

}
