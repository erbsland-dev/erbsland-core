// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "NumberSeparators.hpp"
#include "Sign.hpp"

#include "../decoder/TokenDecoder.hpp"

#include "../../../text/IntegerBase.hpp"

namespace erbsland::conf::impl::lexer {

/// The result of the `parseNumber` call.
class ParseNumberResult final {
public:
    /// Store a parsed integer and its digit count.
    /// @param value The parsed integer value.
    /// @param digitCount The number of parsed digits.
    constexpr ParseNumberResult(int64_t value, std::size_t digitCount) : _value(value), _digitCount(digitCount) {}

    // defaults
    ~ParseNumberResult() = default;

public:
    /// Access the parsed integer value.
    [[nodiscard]] auto value() const noexcept -> int64_t { return _value; }
    /// Access the number of parsed digits.
    [[nodiscard]] auto digitCount() const noexcept -> std::size_t { return _digitCount; }

    /// Assign the parsed integer value to an output variable.
    void assignTo(int64_t &value) const noexcept { value = _value; }

    /// Assign the parsed integer value and digit count to output variables.
    void assignTo(int64_t &value, std::size_t &digitCount) const noexcept {
        value = _value;
        digitCount = _digitCount;
    }

private:
    int64_t _value;
    std::size_t _digitCount;
};

/// Generic parse the number part of a decimal value.
/// @param decoder The decoder to use.
/// @param sign The sign of the parsed value.
/// @param numberSeparators If number separators are allowed.
/// @param base The base for the number.
/// @param fixedDigitCount If a fixed number of digits is expected.
/// @return The parsed number and the number of digits.
/// @throws ConfError If the parsed number exceeds the 64-bit limits, or has a problem with the number separators.
[[nodiscard]] auto parseNumber(
    Decoder &decoder,
    text::IntegerBase base,
    Sign sign,
    NumberSeparators numberSeparators,
    std::size_t fixedDigitCount = 0) -> ParseNumberResult;

}
