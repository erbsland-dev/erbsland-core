// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BigInteger.hpp"

#include "../err/OverflowError.hpp"
#include "../err/ParseError.hpp"
#include "../text/Literals.hpp"
#include "../text/StringEditor.hpp"
#include "../text/u8/U8StringConstIterator.hpp"

#include <utility>

namespace erbsland::math {

using namespace text::literals;

BigInteger::BigInteger(BigUnsignedInteger magnitude) noexcept : _magnitude{std::move(magnitude)} {
}

auto BigInteger::operator<=>(const BigInteger &other) const noexcept -> std::strong_ordering {
    if (_negative != other._negative) {
        return _negative ? std::strong_ordering::less : std::strong_ordering::greater;
    }
    const auto magnitudeOrdering = _magnitude <=> other._magnitude;
    if (!_negative || magnitudeOrdering == std::strong_ordering::equal) {
        return magnitudeOrdering;
    }
    return magnitudeOrdering == std::strong_ordering::less ? std::strong_ordering::greater : std::strong_ordering::less;
}

auto BigInteger::operator+(const BigInteger &other) const -> BigInteger {
    auto result = *this;
    result += other;
    return result;
}

auto BigInteger::operator+=(const BigInteger &other) -> BigInteger & {
    if (_negative == other._negative) {
        _magnitude += other._magnitude;
        return *this;
    }
    const auto ordering = _magnitude <=> other._magnitude;
    if (ordering == std::strong_ordering::equal) {
        _magnitude = {};
        _negative = false;
    } else if (ordering == std::strong_ordering::greater) {
        _magnitude -= other._magnitude;
    } else {
        auto magnitude = other._magnitude - _magnitude;
        _magnitude = std::move(magnitude);
        _negative = other._negative;
    }
    return *this;
}

auto BigInteger::operator-(const BigInteger &other) const -> BigInteger {
    auto result = *this;
    result -= other;
    return result;
}

auto BigInteger::operator-=(const BigInteger &other) -> BigInteger & {
    return *this += other.negated();
}

auto BigInteger::operator-() const -> BigInteger {
    return negated();
}

auto BigInteger::operator*(const BigInteger &other) const -> BigInteger {
    auto result = *this;
    result *= other;
    return result;
}

auto BigInteger::operator*=(const BigInteger &other) -> BigInteger & {
    _magnitude *= other._magnitude;
    _negative = _negative != other._negative;
    normalizeSign();
    return *this;
}

auto BigInteger::operator/(const BigInteger &other) const -> BigInteger {
    auto result = *this;
    result.divideAssign(other);
    return result;
}

auto BigInteger::operator/=(const BigInteger &other) -> BigInteger & {
    divideAssign(other);
    return *this;
}

auto BigInteger::operator%(const BigInteger &other) const -> BigInteger {
    auto quotient = *this;
    return quotient.divideGetRemainder(other);
}

auto BigInteger::operator%=(const BigInteger &other) -> BigInteger & {
    *this = *this % other;
    return *this;
}

auto BigInteger::isZero() const noexcept -> bool {
    return _magnitude.isZero();
}

auto BigInteger::isOne() const noexcept -> bool {
    return !_negative && _magnitude.isOne();
}

auto BigInteger::isNegative() const noexcept -> bool {
    return _negative;
}

auto BigInteger::magnitude() const noexcept -> const BigUnsignedInteger & {
    return _magnitude;
}

auto BigInteger::toString() const -> text::String {
    if (!_negative) {
        return _magnitude.toString();
    }
    auto result = text::StringEditor{"-"_el};
    result.append(_magnitude.toString());
    return result;
}

auto BigInteger::fromString(const text::String &text) -> std::optional<BigInteger> {
    auto magnitude = BigUnsignedInteger{};
    auto negative = false;
    auto isFirst = true;
    auto hasDigit = false;
    const auto loopResult = text.forEach([&](text::Char character) -> util::LoopStatus {
        if (isFirst && (character == U'+' || character == U'-')) {
            negative = character == U'-';
            isFirst = false;
            return util::LoopStatus::Continue;
        }
        isFirst = false;
        const auto digitValue = character.digitValue(text::IntegerBase::Decimal);
        if (!digitValue.has_value()) {
            return util::LoopStatus::Error;
        }
        hasDigit = true;
        magnitude.multiplySmall(10U);
        magnitude.addSmall(*digitValue);
        return util::LoopStatus::Continue;
    });
    if (loopResult == util::LoopResult::Error || !hasDigit) {
        return {};
    }
    return fromSignAndMagnitude(negative, std::move(magnitude));
}

auto BigInteger::fromStringOrThrow(const text::String &text) -> BigInteger {
    auto result = fromString(text);
    if (!result.has_value()) {
        throw err::ParseError{"Invalid signed decimal big integer."};
    }
    return std::move(result).value();
}

auto BigInteger::fromSignAndMagnitude(const bool negative, BigUnsignedInteger magnitude) noexcept -> BigInteger {
    return BigInteger{negative, std::move(magnitude)};
}

auto BigInteger::negated() const -> BigInteger {
    return fromSignAndMagnitude(!_negative, _magnitude);
}

auto BigInteger::divideGetRemainder(const BigInteger &divisor) -> BigInteger {
    const auto dividendWasNegative = _negative;
    const auto quotientIsNegative = _negative != divisor._negative;
    auto remainderMagnitude = _magnitude.divideGetRemainder(divisor._magnitude);
    _negative = quotientIsNegative;
    normalizeSign();
    return fromSignAndMagnitude(dividendWasNegative, std::move(remainderMagnitude));
}

BigInteger::BigInteger(const bool negative, BigUnsignedInteger magnitude) noexcept :
    _magnitude{std::move(magnitude)}, _negative{negative} {
    normalizeSign();
}

void BigInteger::divideAssign(const BigInteger &divisor) {
    _magnitude /= divisor._magnitude;
    _negative = _negative != divisor._negative;
    normalizeSign();
}

void BigInteger::normalizeSign() noexcept {
    if (_magnitude.isZero()) {
        _negative = false;
    }
}

void BigInteger::throwCastOverflow() {
    throw err::OverflowError("BigInteger value cannot be represented by the target type"_el);
}

}
