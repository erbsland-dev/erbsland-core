// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../unit/ByteLength.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <cstdint>

namespace erbsland::mem {

/// The exact wire representation of an integer.
/// @tested{ByteReaderWriterTest}
class ByteIntegerFormat final {
public:
    /// The raw integer wire-format value.
    enum Value : uint8_t {
        UnsignedFixed8Bit,      ///< An unsigned fixed-width 8-bit integer.
        SignedFixed8Bit,        ///< A signed two's-complement fixed-width 8-bit integer.
        UnsignedFixed16Bit,     ///< An unsigned fixed-width 16-bit integer.
        SignedFixed16Bit,       ///< A signed two's-complement fixed-width 16-bit integer.
        UnsignedFixed24Bit,     ///< An unsigned fixed-width 24-bit integer.
        UnsignedFixed32Bit,     ///< An unsigned fixed-width 32-bit integer.
        SignedFixed32Bit,       ///< A signed two's-complement fixed-width 32-bit integer.
        UnsignedFixed40Bit,     ///< An unsigned fixed-width 40-bit integer.
        UnsignedFixed48Bit,     ///< An unsigned fixed-width 48-bit integer.
        UnsignedFixed56Bit,     ///< An unsigned fixed-width 56-bit integer.
        UnsignedFixed64Bit,     ///< An unsigned fixed-width 64-bit integer.
        SignedFixed64Bit,       ///< A signed two's-complement fixed-width 64-bit integer.
        UnsignedVariableLength, ///< An unsigned prefix-coded variable-length integer.
        SignedVariableLength,   ///< A ZigZag-mapped prefix-coded variable-length integer.
        UnsignedBase128,        ///< A canonical unsigned big-endian base-128 integer with continuation bits.

        _valueCount,            ///< The number of wire-format values.
    };

public:
    /// Create a format from its raw value.
    /// @param value The raw wire-format value.
    constexpr ByteIntegerFormat(Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

    // defaults
    constexpr ByteIntegerFormat() noexcept = default;
    ~ByteIntegerFormat() = default;
    ByteIntegerFormat(const ByteIntegerFormat &) = default;
    ByteIntegerFormat(ByteIntegerFormat &&) = default;
    auto operator=(const ByteIntegerFormat &) -> ByteIntegerFormat & = default;
    auto operator=(ByteIntegerFormat &&) -> ByteIntegerFormat & = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const ByteIntegerFormat &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const Value value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(const Value value, const ByteIntegerFormat &other, value, other._value);

public: // accessors
    /// Get the raw wire-format value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }
    /// Get the fixed byte width, or infinite for variable-length formats.
    [[nodiscard]] constexpr auto byteCount() const noexcept -> unit::ByteLength {
        switch (_value) {
        case UnsignedFixed8Bit:
        case SignedFixed8Bit:
            return unit::ByteLength{1U};
        case UnsignedFixed16Bit:
        case SignedFixed16Bit:
            return unit::ByteLength{2U};
        case UnsignedFixed24Bit:
            return unit::ByteLength{3U};
        case UnsignedFixed32Bit:
        case SignedFixed32Bit:
            return unit::ByteLength{4U};
        case UnsignedFixed40Bit:
            return unit::ByteLength{5U};
        case UnsignedFixed48Bit:
            return unit::ByteLength{6U};
        case UnsignedFixed56Bit:
            return unit::ByteLength{7U};
        case UnsignedFixed64Bit:
        case SignedFixed64Bit:
            return unit::ByteLength{8U};
        case UnsignedVariableLength:
        case SignedVariableLength:
        case UnsignedBase128:
            return unit::ByteLength::infinite();
        case _valueCount:
            return unit::ByteLength::zero();
        }
        return unit::ByteLength::zero();
    }
    /// Test if the wire representation is signed.
    [[nodiscard]] constexpr auto isSigned() const noexcept -> bool {
        return _value == SignedFixed8Bit || _value == SignedFixed16Bit || _value == SignedFixed32Bit ||
            _value == SignedFixed64Bit || _value == SignedVariableLength;
    }
    /// Test if the wire representation has a variable byte width.
    [[nodiscard]] constexpr auto isVariableLength() const noexcept -> bool { return byteCount().isInfinite(); }

private:
    Value _value{UnsignedFixed8Bit}; ///< The raw wire-format value.
};

}
