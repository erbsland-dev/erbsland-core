// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Number.hpp"

#include "../char/NamedChars.hpp"
#include "../constants/Limits.hpp"
#include "../utilities/YieldMacros.hpp"

#include "../../../math/SaturatingMath.hpp"

namespace erbsland::conf::impl::lexer {

using namespace text::literals;

/// Helper for `parseNumber`.
/// @param decoder The decoder.
/// @param base The base of the number to parse.
/// @param digitCount The number of digits.
void handleDigitSeparator(Decoder &decoder, const text::IntegerBase base, const std::size_t digitCount) {
    if (decoder.character() == nc::digitSeparator) {
        if (digitCount == 0) {
            decoder.throwSyntaxError("Number cannot start with a digit separator."_el);
        }
        decoder.next(); // Skip it, but expect another digit.
        if (decoder.character() == nc::digitSeparator) {
            decoder.throwSyntaxError("Number cannot contain two consecutive digit separators."_el);
        }
        if (!decoder.character().isDigitValue(base)) {
            decoder.throwSyntaxOrUnexpectedEndError("Expected another digit after the digit separator."_el);
        }
    }
}

auto parseNumber(
    Decoder &decoder,
    const text::IntegerBase base,
    const Sign sign,
    const NumberSeparators numberSeparators,
    const std::size_t fixedDigitCount) -> ParseNumberResult {

    if (decoder.character().isEndOfData()) {
        decoder.throwUnexpectedEndOfDataError("Expected a number, but the document ended at this point."_el);
    }
    uint64_t value = 0;
    std::size_t digitCount = 0;
    while (!decoder.character().isEndOfData()) {
        if (fixedDigitCount > 0 && digitCount >= fixedDigitCount) {
            break;
        }
        if (numberSeparators == NumberSeparators::Yes) {
            handleDigitSeparator(decoder, base, digitCount);
        }
        const auto digit = decoder.character().digitValue(base);
        if (digit.has_value()) {
            if (digitCount >= limits::maximumDigits(base)) {
                decoder.throwNumberLimitExceededError();
            }
            const auto digitValue = static_cast<uint64_t>(digit.value());
            if (math::willMultiplyOverflow(value, base.baseFactor())) {
                decoder.throwNumberLimitExceededError();
            }
            value *= base.baseFactor();
            if (math::willAddOverflow(value, digitValue)) {
                decoder.throwNumberLimitExceededError();
            }
            value += digitValue;
        } else {
            break;
        }
        ++digitCount;
        decoder.next();
    }
    decoder.checkForErrorAndThrowIt(); // Check if the number parsing was stopped because of an error.
    if (fixedDigitCount > 0 && digitCount < fixedDigitCount) {
        // For a fixed digit count, return -1 instead of throwing an error, as this is used to test numbers
        // and backtrack if the number does not have the expected number of digits.
        return {-1, digitCount};
    }
    // Checks if the number is in the required 64-bit limits.
    if (sign == Sign::Negative) {
        const auto negativeLimit = static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1U;
        if (value > negativeLimit) {
            decoder.throwNumberLimitExceededError();
        }
        // Special handling of the largest negative number to avoid undefined behavior.
        if (value == negativeLimit) {
            return {std::numeric_limits<int64_t>::min(), digitCount};
        }
        return {-static_cast<int64_t>(value), digitCount};
    }
    if (value > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
        decoder.throwNumberLimitExceededError();
    }
    return {static_cast<int64_t>(value), digitCount};
}

}
