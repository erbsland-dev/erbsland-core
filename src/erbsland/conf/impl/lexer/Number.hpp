// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../decoder/TokenDecoder.hpp"

#include "../../../text/IntegerBase.hpp"

namespace erbsland::conf::impl::lexer {

/// The result of the `parseNumber` call.
class ParseNumberResult final {
public:
    constexpr ParseNumberResult(int64_t value, std::size_t digitCount) : _value(value), _digitCount(digitCount) {}
    ~ParseNumberResult() = default;

public:
    [[nodiscard]] auto value() const noexcept -> int64_t { return _value; }
    [[nodiscard]] auto digitCount() const noexcept -> std::size_t { return _digitCount; }

    void assignTo(int64_t &value) const noexcept { value = _value; }

    void assignTo(int64_t &value, std::size_t &digitCount) const noexcept {
        value = _value;
        digitCount = _digitCount;
    }

private:
    int64_t _value;
    std::size_t _digitCount;
};

/// If number separators are accepted.
enum class NumberSeparators : uint8_t { Yes, No };

/// The sign of a number.
enum class Sign : uint8_t { Positive, Negative };

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
