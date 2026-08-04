// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BigUnsignedInteger.hpp"

#include "../err/OverflowError.hpp"
#include "../err/ParseError.hpp"
#include "../text/Literals.hpp"
#include "../text/StringEditor.hpp"
#include "../text/StringFormat.hpp"
#include "../text/u8/U8StringConstIterator.hpp"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <utility>

namespace erbsland::math {

using namespace text::literals;

auto BigUnsignedInteger::operator<=>(const BigUnsignedInteger &other) const noexcept -> std::strong_ordering {
    if (_digits.size() != other._digits.size()) {
        return _digits.size() < other._digits.size() ? std::strong_ordering::less : std::strong_ordering::greater;
    }
    for (auto index = _digits.size(); index > 0; --index) {
        if (_digits[index - 1] != other._digits[index - 1]) {
            return _digits[index - 1] < other._digits[index - 1] ? std::strong_ordering::less
                                                                 : std::strong_ordering::greater;
        }
    }
    return std::strong_ordering::equal;
}

auto BigUnsignedInteger::operator+(const BigUnsignedInteger &other) const -> BigUnsignedInteger {
    auto result = *this;
    result += other;
    return result;
}

auto BigUnsignedInteger::operator+=(const BigUnsignedInteger &other) -> BigUnsignedInteger & {
    const auto digitCount = std::max(_digits.size(), other._digits.size());
    _digits.resize(digitCount, 0U);
    std::uint64_t carry = 0;
    for (std::size_t index = 0; index < digitCount; ++index) {
        const auto otherDigit = index < other._digits.size() ? other._digits[index] : 0U;
        const auto sum = static_cast<std::uint64_t>(_digits[index]) + otherDigit + carry;
        _digits[index] = static_cast<std::uint32_t>(sum % cDigitBase);
        carry = sum / cDigitBase;
    }
    if (carry != 0) {
        _digits.push_back(static_cast<std::uint32_t>(carry));
    }
    return *this;
}

auto BigUnsignedInteger::operator-(const BigUnsignedInteger &other) const -> BigUnsignedInteger {
    auto result = *this;
    result -= other;
    return result;
}

auto BigUnsignedInteger::operator-=(const BigUnsignedInteger &other) -> BigUnsignedInteger & {
    if (*this < other) {
        throwSubtractionUnderflow();
    }
    std::int64_t borrow = 0;
    for (std::size_t index = 0; index < _digits.size(); ++index) {
        const auto otherDigit = index < other._digits.size() ? other._digits[index] : 0U;
        auto difference = static_cast<std::int64_t>(_digits[index]) - otherDigit - borrow;
        if (difference < 0) {
            difference += cDigitBase;
            borrow = 1;
        } else {
            borrow = 0;
        }
        _digits[index] = static_cast<std::uint32_t>(difference);
    }
    trim();
    return *this;
}

auto BigUnsignedInteger::operator*(const BigUnsignedInteger &other) const -> BigUnsignedInteger {
    auto result = *this;
    result *= other;
    return result;
}

auto BigUnsignedInteger::operator*=(const BigUnsignedInteger &other) -> BigUnsignedInteger & {
    if (isZero() || other.isZero()) {
        _digits.clear();
        return *this;
    }
    auto result = std::vector<std::uint32_t>(_digits.size() + other._digits.size(), 0U);
    for (std::size_t left = 0; left < _digits.size(); ++left) {
        std::uint64_t carry = 0;
        for (std::size_t right = 0; right < other._digits.size(); ++right) {
            const auto index = left + right;
            const auto product =
                static_cast<std::uint64_t>(_digits[left]) * other._digits[right] + result[index] + carry;
            result[index] = static_cast<std::uint32_t>(product % cDigitBase);
            carry = product / cDigitBase;
        }
        auto index = left + other._digits.size();
        while (carry != 0) {
            const auto sum = static_cast<std::uint64_t>(result[index]) + carry;
            result[index] = static_cast<std::uint32_t>(sum % cDigitBase);
            carry = sum / cDigitBase;
            ++index;
        }
    }
    _digits = std::move(result);
    trim();
    return *this;
}

auto BigUnsignedInteger::operator/(const BigUnsignedInteger &other) const -> BigUnsignedInteger {
    auto quotient = BigUnsignedInteger{};
    divideWithRemainder(*this, other, quotient);
    return quotient;
}

auto BigUnsignedInteger::operator/=(const BigUnsignedInteger &other) -> BigUnsignedInteger & {
    auto quotient = BigUnsignedInteger{};
    divideWithRemainder(*this, other, quotient);
    *this = std::move(quotient);
    return *this;
}

auto BigUnsignedInteger::operator%(const BigUnsignedInteger &other) const -> BigUnsignedInteger {
    auto quotient = BigUnsignedInteger{};
    return divideWithRemainder(*this, other, quotient);
}

auto BigUnsignedInteger::operator%=(const BigUnsignedInteger &other) -> BigUnsignedInteger & {
    *this = *this % other;
    return *this;
}

auto BigUnsignedInteger::isZero() const noexcept -> bool {
    return _digits.empty();
}

auto BigUnsignedInteger::isOne() const noexcept -> bool {
    return _digits.size() == 1 && _digits.front() == 1U;
}

auto BigUnsignedInteger::toString() const -> text::String {
    static const auto cDigitGroupFormat = text::StringFormat{"{:09}"_el};
    if (isZero()) {
        return text::String{"0"_el};
    }
    auto result = text::StringEditor::fromInteger(_digits.back());
    for (auto index = _digits.size() - 1; index > 0; --index) {
        result.append(cDigitGroupFormat.build(_digits[index - 1]));
    }
    return result;
}

auto BigUnsignedInteger::fromString(const text::String &text) -> std::optional<BigUnsignedInteger> {
    auto result = BigUnsignedInteger{};
    auto isFirst = true;
    auto hasDigit = false;
    const auto loopResult = text.forEach([&](text::Char character) -> util::LoopStatus {
        if (isFirst && character == U'+') {
            isFirst = false;
            return util::LoopStatus::Continue;
        }
        isFirst = false;
        const auto digitValue = character.digitValue(text::IntegerBase::Decimal);
        if (!digitValue.has_value()) {
            return util::LoopStatus::Error;
        }
        hasDigit = true;
        result.multiplySmall(10U);
        result.addSmall(*digitValue);
        return util::LoopStatus::Continue;
    });
    if (loopResult == util::LoopResult::Error || !hasDigit) {
        return {};
    }
    return result;
}

auto BigUnsignedInteger::fromStringOrThrow(const text::String &text) -> BigUnsignedInteger {
    auto result = fromString(text);
    if (!result.has_value()) {
        throw err::ParseError{"Invalid unsigned decimal big integer."_el};
    }
    return std::move(result).value();
}

auto BigUnsignedInteger::divideGetRemainder(const BigUnsignedInteger &divisor) -> BigUnsignedInteger {
    auto quotient = BigUnsignedInteger{};
    auto remainder = divideWithRemainder(*this, divisor, quotient);
    *this = std::move(quotient);
    return remainder;
}

void BigUnsignedInteger::assign(const std::uint64_t value) {
    auto remaining = value;
    while (remaining != 0) {
        _digits.push_back(static_cast<std::uint32_t>(remaining % cDigitBase));
        remaining /= cDigitBase;
    }
}

void BigUnsignedInteger::addSmall(const std::uint32_t value) {
    std::uint64_t carry = value;
    for (std::size_t index = 0; index < _digits.size() && carry != 0; ++index) {
        const auto sum = static_cast<std::uint64_t>(_digits[index]) + carry;
        _digits[index] = static_cast<std::uint32_t>(sum % cDigitBase);
        carry = sum / cDigitBase;
    }
    if (carry != 0) {
        _digits.push_back(static_cast<std::uint32_t>(carry));
    }
}

void BigUnsignedInteger::multiplySmall(const std::uint32_t value) {
    if (value == 0U || isZero()) {
        _digits.clear();
        return;
    }
    std::uint64_t carry = 0;
    for (auto &digit : _digits) {
        const auto product = static_cast<std::uint64_t>(digit) * value + carry;
        digit = static_cast<std::uint32_t>(product % cDigitBase);
        carry = product / cDigitBase;
    }
    if (carry != 0) {
        _digits.push_back(static_cast<std::uint32_t>(carry));
    }
}

auto BigUnsignedInteger::divideSmall(const std::uint32_t divisor) noexcept -> std::uint32_t {
    std::uint64_t remainder = 0;
    for (auto index = _digits.size(); index > 0; --index) {
        const auto value = remainder * cDigitBase + _digits[index - 1];
        _digits[index - 1] = static_cast<std::uint32_t>(value / divisor);
        remainder = value % divisor;
    }
    trim();
    return static_cast<std::uint32_t>(remainder);
}

auto BigUnsignedInteger::toUInt64Unchecked() const noexcept -> std::uint64_t {
    std::uint64_t result = 0;
    for (auto index = _digits.size(); index > 0; --index) {
        result = result * cDigitBase + _digits[index - 1];
    }
    return result;
}

void BigUnsignedInteger::trim() noexcept {
    while (!_digits.empty() && _digits.back() == 0U) {
        _digits.pop_back();
    }
}

auto BigUnsignedInteger::divideWithRemainder(
    const BigUnsignedInteger &dividend, const BigUnsignedInteger &divisor, BigUnsignedInteger &quotient)
    -> BigUnsignedInteger {
    if (divisor.isZero()) {
        std::terminate();
    }
    if (dividend < divisor) {
        quotient = {};
        return dividend;
    }
    if (divisor._digits.size() == 1U) {
        quotient = dividend;
        const auto smallRemainder = quotient.divideSmall(divisor._digits.front());
        return BigUnsignedInteger{smallRemainder};
    }

    const auto normalization = static_cast<std::uint32_t>(
        static_cast<std::uint64_t>(cDigitBase) / (static_cast<std::uint64_t>(divisor._digits.back()) + 1U));
    auto normalizedDividend = dividend;
    auto normalizedDivisor = divisor;
    normalizedDividend.multiplySmall(normalization);
    normalizedDivisor.multiplySmall(normalization);
    normalizedDividend._digits.push_back(0U);

    const auto divisorSize = normalizedDivisor._digits.size();
    const auto quotientSize = normalizedDividend._digits.size() - divisorSize;
    quotient._digits.assign(quotientSize, 0U);

    for (auto offset = quotientSize; offset > 0; --offset) {
        const auto quotientIndex = offset - 1;
        const auto high = normalizedDividend._digits[quotientIndex + divisorSize];
        const auto next = normalizedDividend._digits[quotientIndex + divisorSize - 1];
        const auto estimateSource = static_cast<std::uint64_t>(high) * cDigitBase + next;
        auto estimatedDigit = estimateSource / normalizedDivisor._digits.back();
        auto estimatedRemainder = estimateSource % normalizedDivisor._digits.back();
        if (estimatedDigit >= cDigitBase) {
            estimatedDigit = cDigitBase - 1U;
            estimatedRemainder += normalizedDivisor._digits.back();
        }
        while (
            estimatedRemainder < cDigitBase &&
            estimatedDigit * normalizedDivisor._digits[divisorSize - 2] >
                estimatedRemainder * cDigitBase + normalizedDividend._digits[quotientIndex + divisorSize - 2]) {
            --estimatedDigit;
            estimatedRemainder += normalizedDivisor._digits.back();
        }

        std::int64_t carry = 0;
        for (std::size_t index = 0; index < divisorSize; ++index) {
            auto difference = static_cast<std::int64_t>(normalizedDividend._digits[quotientIndex + index]) -
                static_cast<std::int64_t>(estimatedDigit * normalizedDivisor._digits[index]) - carry;
            if (difference < 0) {
                carry = (-difference + cDigitBase - 1U) / cDigitBase;
                difference += carry * cDigitBase;
            } else {
                carry = 0;
            }
            normalizedDividend._digits[quotientIndex + index] = static_cast<std::uint32_t>(difference);
        }
        auto highDifference =
            static_cast<std::int64_t>(normalizedDividend._digits[quotientIndex + divisorSize]) - carry;
        if (highDifference < 0) {
            --estimatedDigit;
            std::uint64_t additionCarry = 0;
            for (std::size_t index = 0; index < divisorSize; ++index) {
                const auto sum = static_cast<std::uint64_t>(normalizedDividend._digits[quotientIndex + index]) +
                    normalizedDivisor._digits[index] + additionCarry;
                normalizedDividend._digits[quotientIndex + index] = static_cast<std::uint32_t>(sum % cDigitBase);
                additionCarry = sum / cDigitBase;
            }
            highDifference += static_cast<std::int64_t>(additionCarry);
        }
        normalizedDividend._digits[quotientIndex + divisorSize] = static_cast<std::uint32_t>(highDifference);
        quotient._digits[quotientIndex] = static_cast<std::uint32_t>(estimatedDigit);
    }

    quotient.trim();
    auto remainder = BigUnsignedInteger{};
    remainder._digits.assign(
        normalizedDividend._digits.begin(),
        normalizedDividend._digits.begin() + static_cast<std::ptrdiff_t>(divisorSize));
    if (remainder.divideSmall(normalization) != 0U) {
        std::terminate();
    }
    remainder.trim();
    return remainder;
}

void BigUnsignedInteger::throwNegativeValue() {
    throw err::OverflowError("A BigUnsignedInteger cannot represent a negative value"_el);
}

void BigUnsignedInteger::throwSubtractionUnderflow() {
    throw err::OverflowError("BigUnsignedInteger subtraction would produce a negative value"_el);
}

void BigUnsignedInteger::throwCastOverflow() {
    throw err::OverflowError("BigUnsignedInteger value cannot be represented by the target type"_el);
}

}
