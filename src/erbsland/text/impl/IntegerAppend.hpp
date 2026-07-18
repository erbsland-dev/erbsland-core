// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerAppend_fwd.hpp"

#include "../Char.hpp"
#include "../IntegerFormat.hpp"

#include "../../math/SignedMagnitude.hpp"

#include <array>
#include <cstddef>
#include <limits>

namespace erbsland::text::impl {

/// Append a grouped digit field to a decoded-character sink.
template <typename tSink, typename tDigits>
void appendDigitField(
    tSink &sink,
    const tDigits &digits,
    const std::size_t digitCount,
    const std::size_t zeroPadCount,
    const IntegerFormat &format) {
    const auto base = format.base();
    const auto useSeparator = format.hasFlag(IntegerFormatFlag::Separator);
    const auto groupSize = base.digitGroupSize();
    const auto paddedDigitCount = digitCount + zeroPadCount;
    auto firstGroupSize = paddedDigitCount % groupSize;
    if (firstGroupSize == 0U) {
        firstGroupSize = groupSize;
    }

    for (auto index = std::size_t{0U}; index < paddedDigitCount; ++index) {
        if (useSeparator && index > 0U && index >= firstGroupSize && ((index - firstGroupSize) % groupSize) == 0U) {
            sink.append(Char{U'\''});
        }
        if (index < zeroPadCount) {
            sink.append(Char{U'0'});
        } else {
            const auto digitIndex = digitCount - 1U - (index - zeroPadCount);
            sink.append(Char{digits[digitIndex]});
        }
    }
}

/// Append an integer to a decoded-character sink.
template <typename tSink, math::AnyIntegerType T>
void appendInteger(tSink &sink, T value, const IntegerFormat &format) {
    using NativeValue = math::NativeIntegerOfT<T>;
    using Magnitude = math::SignedMagnitude<NativeValue>;
    using UnsignedValue = typename Magnitude::Unsigned;

    const auto magnitude = Magnitude{value};
    const auto negative = magnitude.isNegative();
    auto remainingMagnitude = magnitude.magnitude();
    const auto baseValue = static_cast<UnsignedValue>(format.base().baseFactor());
    constexpr auto cMaxDigits = std::numeric_limits<UnsignedValue>::digits + 1U;
    auto digits = std::array<char32_t, cMaxDigits>{};
    auto digitCount = std::size_t{0U};

    do {
        const auto digit = static_cast<unsigned int>(remainingMagnitude % baseValue);
        digits[digitCount] = Char::fromDigitValue(digit, format.letterCase()).toRawValue();
        ++digitCount;
        remainingMagnitude = static_cast<UnsignedValue>(remainingMagnitude / baseValue);
    } while (remainingMagnitude != 0U);

    const auto precision = format.hasPrecision() ? format.precision().toSizeTOrThrow() : std::size_t{0U};
    const auto precisionZeroPadCount = precision > digitCount ? precision - digitCount : std::size_t{0U};
    const auto precisionDigitCount = digitCount + precisionZeroPadCount;
    const auto fieldWidth = format.fieldWidth().toSizeTOrThrow();
    const auto missingDigits = fieldWidth > precisionDigitCount ? fieldWidth - precisionDigitCount : std::size_t{0U};
    const auto fieldZeroPadCount = format.hasFlag(IntegerFormatFlag::ZeroFill) ? missingDigits : std::size_t{0U};
    const auto zeroPadCount = precisionZeroPadCount + fieldZeroPadCount;
    const auto spacePadCount = format.hasFlag(IntegerFormatFlag::ZeroFill) ? std::size_t{0U} : missingDigits;

    for (auto i = std::size_t{0U}; i < spacePadCount; ++i) {
        sink.append(Char{U' '});
    }
    if (negative) {
        sink.append(Char{U'-'});
    } else if (format.signMode() == IntegerSignMode::Always) {
        sink.append(Char{U'+'});
    } else if (format.signMode() == IntegerSignMode::Space) {
        sink.append(Char{U' '});
    }
    if (format.hasFlag(IntegerFormatFlag::BasePrefix)) {
        const auto prefix = format.base().prefixChar(format.letterCase());
        if (!prefix.isNull()) {
            sink.append(Char{U'0'});
            sink.append(prefix);
        }
    }
    appendDigitField(sink, digits, digitCount, zeroPadCount, format);
}

}
