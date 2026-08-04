// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Char_fwd.hpp"
#include "LetterCase.hpp"
#include "StringEditor_fwd.hpp"

#include "../util/impl/ComparisonHelper.hpp"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <type_traits>

namespace erbsland::text {

/// The base used for integer text conversion.
/// @tested{IntegerConversionTest}
class IntegerBase final {
public:
    /// The integer base value.
    enum Value : uint8_t {
        Decimal = 0,     ///< Decimal base 10.
        Hexadecimal = 1, ///< Hexadecimal base 16.
        Binary = 2,      ///< Binary base 2.
        Octal = 3,       ///< Octal base 8.
    };

public:
    /// Create an integer base from a value.
    constexpr IntegerBase(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

    // defaults
    constexpr IntegerBase() noexcept = default;
    ~IntegerBase() = default;
    IntegerBase(const IntegerBase &) = default;
    IntegerBase(IntegerBase &&) = default;
    auto operator=(const IntegerBase &) -> IntegerBase & = default;
    auto operator=(IntegerBase &&) -> IntegerBase & = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const IntegerBase &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const Value value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(const Value value, const IntegerBase &other, value, other._value);

public: // accessors
    /// Get the raw integer base value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }
    /// Get the numeric factor for this base.
    [[nodiscard]] constexpr auto baseFactor() const noexcept -> unsigned int {
        switch (_value) {
        case Decimal:
            return 10U;
        case Hexadecimal:
            return 16U;
        case Binary:
            return 2U;
        case Octal:
            return 8U;
        }
        return 10U;
    }
    /// Get the digit group size for separators.
    [[nodiscard]] constexpr auto digitGroupSize() const noexcept -> std::size_t {
        return _value == Hexadecimal || _value == Binary ? std::size_t{4U} : std::size_t{3U};
    }
    /// Count the digits required to represent the magnitude of an integer value in this base.
    /// The sign is not counted for signed negative values.
    template <std::integral T>
    [[nodiscard]] constexpr auto digitCount(T value) const noexcept -> std::size_t;
    /// Get the ASCII prefix character for this base, or a null character if the base has no prefix.
    [[nodiscard]] auto prefixChar(LetterCase letterCase) const noexcept -> Char;

public: // conversion
    /// Convert this integer base to its canonical name.
    [[nodiscard]] auto toString() const noexcept -> String;

public: // factories
    /// Create an integer base from an ASCII prefix character.
    [[nodiscard]] static auto fromPrefixChar(Char character) noexcept -> std::optional<IntegerBase>;

private:
    Value _value{Decimal}; ///< The integer base value.
};

/// Convenient constant for decimal integer conversion.
inline constexpr auto cDecimalBase = IntegerBase{IntegerBase::Decimal};
/// Convenient constant for hexadecimal integer conversion.
inline constexpr auto cHexadecimalBase = IntegerBase{IntegerBase::Hexadecimal};
/// Convenient constant for binary integer conversion.
inline constexpr auto cBinaryBase = IntegerBase{IntegerBase::Binary};
/// Convenient constant for octal integer conversion.
inline constexpr auto cOctalBase = IntegerBase{IntegerBase::Octal};

/// Count the digits required to represent an integer in this base.
template <std::integral T>
constexpr auto IntegerBase::digitCount(const T value) const noexcept -> std::size_t {
    using Unsigned = std::make_unsigned_t<T>;
    auto magnitude = Unsigned{};
    if constexpr (std::signed_integral<T>) {
        if (value < 0) {
            magnitude = static_cast<Unsigned>(-(value + 1));
            ++magnitude;
        } else {
            magnitude = static_cast<Unsigned>(value);
        }
    } else {
        magnitude = static_cast<Unsigned>(value);
    }

    const auto base = static_cast<Unsigned>(baseFactor());
    auto result = std::size_t{1U};
    while (magnitude >= base) {
        magnitude = static_cast<Unsigned>(magnitude / base);
        ++result;
    }
    return result;
}

}
