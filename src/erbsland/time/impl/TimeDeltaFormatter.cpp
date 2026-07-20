// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TimeDeltaFormatter.hpp"

#include "../../text/Literals.hpp"
#include "../../text/StringFormat.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace erbsland::time::impl {

using namespace text::literals;

namespace {

struct UnitDefinition {
    TimeDeltaUnit unit;
    int64_t factor;
    text::StringLiteral shortName;
    text::StringLiteral elclName;
    text::StringLiteral singular;
    text::StringLiteral plural;
};

constexpr auto cUnits = std::array{
    UnitDefinition{TimeDeltaUnit::Weeks, 604800000000000LL, "w"_el, "w"_el, "week"_el, "weeks"_el},
    UnitDefinition{TimeDeltaUnit::Days, 86400000000000LL, "d"_el, "d"_el, "day"_el, "days"_el},
    UnitDefinition{TimeDeltaUnit::Hours, 3600000000000LL, "h"_el, "h"_el, "hour"_el, "hours"_el},
    UnitDefinition{TimeDeltaUnit::Minutes, 60000000000LL, "min"_el, "m"_el, "minute"_el, "minutes"_el},
    UnitDefinition{TimeDeltaUnit::Seconds, 1000000000LL, "s"_el, "s"_el, "second"_el, "seconds"_el},
    UnitDefinition{TimeDeltaUnit::Milliseconds, 1000000LL, "ms"_el, "ms"_el, "millisecond"_el, "milliseconds"_el},
    UnitDefinition{TimeDeltaUnit::Microseconds, 1000LL, "us"_el, "us"_el, "microsecond"_el, "microseconds"_el},
    UnitDefinition{TimeDeltaUnit::Nanoseconds, 1LL, "ns"_el, "ns"_el, "nanosecond"_el, "nanoseconds"_el},
};

class BigMagnitude {
    static constexpr uint32_t cBase = 1'000'000'000U;

public:
    BigMagnitude() = default;
    explicit BigMagnitude(uint64_t value) {
        while (value != 0) {
            _digits.push_back(static_cast<uint32_t>(value % cBase));
            value /= cBase;
        }
    }

    [[nodiscard]] auto isZero() const noexcept -> bool { return _digits.empty(); }
    [[nodiscard]] auto isOne() const noexcept -> bool { return _digits.size() == 1 && _digits.front() == 1; }

    [[nodiscard]] auto compare(const BigMagnitude &other) const noexcept -> int {
        if (_digits.size() != other._digits.size()) {
            return _digits.size() < other._digits.size() ? -1 : 1;
        }
        for (auto index = _digits.size(); index > 0; --index) {
            if (_digits[index - 1] != other._digits[index - 1]) {
                return _digits[index - 1] < other._digits[index - 1] ? -1 : 1;
            }
        }
        return 0;
    }

    void add(const BigMagnitude &other) {
        const auto count = std::max(_digits.size(), other._digits.size());
        _digits.resize(count, 0);
        uint64_t carry = 0;
        for (std::size_t index = 0; index < count; ++index) {
            const auto otherDigit = index < other._digits.size() ? other._digits[index] : 0U;
            const auto sum = static_cast<uint64_t>(_digits[index]) + otherDigit + carry;
            _digits[index] = static_cast<uint32_t>(sum % cBase);
            carry = sum / cBase;
        }
        if (carry != 0) {
            _digits.push_back(static_cast<uint32_t>(carry));
        }
    }

    void subtract(const BigMagnitude &other) {
        int64_t borrow = 0;
        for (std::size_t index = 0; index < _digits.size(); ++index) {
            const auto otherDigit = index < other._digits.size() ? other._digits[index] : 0U;
            auto difference = static_cast<int64_t>(_digits[index]) - static_cast<int64_t>(otherDigit) - borrow;
            if (difference < 0) {
                difference += cBase;
                borrow = 1;
            } else {
                borrow = 0;
            }
            _digits[index] = static_cast<uint32_t>(difference);
        }
        trim();
    }

    void multiply(const BigMagnitude &other) {
        if (isZero() || other.isZero()) {
            _digits.clear();
            return;
        }
        auto result = std::vector<uint32_t>(_digits.size() + other._digits.size(), 0U);
        for (std::size_t left = 0; left < _digits.size(); ++left) {
            uint64_t carry = 0;
            for (std::size_t right = 0; right < other._digits.size(); ++right) {
                const auto index = left + right;
                const auto product =
                    static_cast<uint64_t>(_digits[left]) * other._digits[right] + result[index] + carry;
                result[index] = static_cast<uint32_t>(product % cBase);
                carry = product / cBase;
            }
            auto index = left + other._digits.size();
            while (carry != 0) {
                const auto sum = static_cast<uint64_t>(result[index]) + carry;
                result[index] = static_cast<uint32_t>(sum % cBase);
                carry = sum / cBase;
                ++index;
                if (index == result.size() && carry != 0) {
                    result.push_back(0U);
                }
            }
        }
        _digits = std::move(result);
        trim();
    }

    [[nodiscard]] auto divide(const uint32_t divisor) noexcept -> uint32_t {
        uint64_t remainder = 0;
        for (auto index = _digits.size(); index > 0; --index) {
            const auto value = remainder * cBase + _digits[index - 1];
            _digits[index - 1] = static_cast<uint32_t>(value / divisor);
            remainder = value % divisor;
        }
        trim();
        return static_cast<uint32_t>(remainder);
    }

    [[nodiscard]] auto toString() const -> text::String {
        static const auto digitGroupFormat = text::StringFormat{"{:09}"_el};
        if (isZero()) {
            return text::String{"0"_el};
        }
        auto result = text::StringEditor::fromInteger(_digits.back());
        for (auto index = _digits.size() - 1; index > 0; --index) {
            result.append(digitGroupFormat.build(_digits[index - 1]));
        }
        return result;
    }

private:
    void trim() noexcept {
        while (!_digits.empty() && _digits.back() == 0) {
            _digits.pop_back();
        }
    }

    std::vector<uint32_t> _digits;
};

class SignedBigInteger {
public:
    void addProduct(const int64_t value, const uint64_t factor) {
        if (value == 0) {
            return;
        }
        const auto magnitude = value < 0 ? static_cast<uint64_t>(-(value + 1)) + 1U : static_cast<uint64_t>(value);
        auto term = BigMagnitude{magnitude};
        term.multiply(BigMagnitude{factor});
        const auto negative = value < 0;
        if (_magnitude.isZero()) {
            _magnitude = std::move(term);
            _negative = negative;
        } else if (_negative == negative) {
            _magnitude.add(term);
        } else {
            const auto comparison = _magnitude.compare(term);
            if (comparison >= 0) {
                _magnitude.subtract(term);
            } else {
                term.subtract(_magnitude);
                _magnitude = std::move(term);
                _negative = negative;
            }
        }
        if (_magnitude.isZero()) {
            _negative = false;
        }
    }

    [[nodiscard]] auto isZero() const noexcept -> bool { return _magnitude.isZero(); }
    [[nodiscard]] auto isNegative() const noexcept -> bool { return _negative; }
    [[nodiscard]] auto magnitudeIsOne() const noexcept -> bool { return _magnitude.isOne(); }
    [[nodiscard]] auto divide(const uint32_t divisor) noexcept -> uint32_t { return _magnitude.divide(divisor); }
    [[nodiscard]] auto magnitudeText() const -> text::String { return _magnitude.toString(); }

private:
    BigMagnitude _magnitude;
    bool _negative{false};
};

auto unitDefinition(const TimeDeltaUnit unit) noexcept -> const UnitDefinition & {
    for (const auto &definition : cUnits) {
        if (definition.unit == unit) {
            return definition;
        }
    }
    return cUnits.back();
}

void appendSeparator(text::StringEditor &result, const TimeDeltaFormat &format) {
    if (!result.isEmpty()) {
        result.append(format.unitSeparator());
    }
}

void appendUnitName(
    text::StringEditor &result,
    const int64_t value,
    const bool hasFraction,
    const UnitDefinition &definition,
    const TimeDeltaFormat &format) {
    result.append(format.valueSeparator());
    if (format.unitStyle() == TimeDeltaFormat::UnitStyle::Long) {
        result.append(!hasFraction && (value == 1 || value == -1) ? definition.singular : definition.plural);
    } else {
        result.append(format.usesElclUnitNames() ? definition.elclName : definition.shortName);
    }
}

void appendFraction(text::StringEditor &result, int64_t remainder, const int64_t factor, const uint8_t maximumDigits) {
    static const auto zeroCharacters = text::CharSet{U'0'};
    if (remainder == 0 || maximumDigits == 0) {
        return;
    }
    if (remainder < 0) {
        remainder = -remainder;
    }
    auto fraction = text::StringEditor{};
    for (uint8_t index = 0; index < maximumDigits && remainder != 0; ++index) {
        remainder *= 10;
        fraction.append(text::Char{static_cast<char32_t>(U'0' + remainder / factor)});
        remainder %= factor;
    }
    fraction.trim(zeroCharacters, text::StringSide::Back);
    if (!fraction.isEmpty()) {
        result.append(U'.');
        result.append(fraction);
    }
}

}

auto formatTimeDelta(const Nanoseconds value, const TimeDeltaFormat &format, const bool includeZero) -> text::String {
    auto result = text::StringEditor{};
    auto remaining = value.toRawValue();
    const auto &smallest = unitDefinition(format.smallestUnit());
    for (const auto &definition : cUnits) {
        if (definition.factor < smallest.factor) {
            continue;
        }
        const auto component = remaining / definition.factor;
        remaining %= definition.factor;
        const auto isSmallest = definition.unit == smallest.unit;
        const auto hasFraction = isSmallest && format.showFractions() && remaining != 0;
        if (component != 0 || hasFraction) {
            appendSeparator(result, format);
            if (component == 0 && remaining < 0) {
                result.append(U'-');
            }
            result.append(text::String::fromInteger(component));
            if (isSmallest && format.showFractions()) {
                appendFraction(result, remaining, definition.factor, format.maximumFractionDigits());
            }
            appendUnitName(result, component, hasFraction, definition, format);
        }
        if (isSmallest) {
            break;
        }
    }
    if (result.isEmpty() && includeZero) {
        const auto &zeroUnit = unitDefinition(
            smallest.factor > unitDefinition(TimeDeltaUnit::Seconds).factor ? smallest.unit : TimeDeltaUnit::Seconds);
        result.append(text::String::fromInteger(0));
        appendUnitName(result, 0, false, zeroUnit, format);
    }
    return result;
}

auto formatCalendarFixedDelta(
    const Nanoseconds nanoseconds,
    const Microseconds microseconds,
    const Milliseconds milliseconds,
    const Seconds seconds,
    const Minutes minutes,
    const Hours hours,
    const Days days,
    const Weeks weeks,
    const TimeDeltaFormat &format,
    const bool includeZero) -> text::String {
    constexpr auto factors = std::array<uint64_t, 8>{
        1ULL,
        1'000ULL,
        1'000'000ULL,
        1'000'000'000ULL,
        60'000'000'000ULL,
        3'600'000'000'000ULL,
        86'400'000'000'000ULL,
        604'800'000'000'000ULL};
    auto total = SignedBigInteger{};
    total.addProduct(nanoseconds.toRawValue(), factors[0]);
    total.addProduct(microseconds.toRawValue(), factors[1]);
    total.addProduct(milliseconds.toRawValue(), factors[2]);
    total.addProduct(seconds.toRawValue(), factors[3]);
    total.addProduct(minutes.toRawValue(), factors[4]);
    total.addProduct(hours.toRawValue(), factors[5]);
    total.addProduct(days.toRawValue(), factors[6]);
    total.addProduct(weeks.toRawValue(), factors[7]);

    const auto negative = total.isNegative();
    auto components = std::array<uint32_t, 7>{};
    components[0] = total.divide(1000U);
    components[1] = total.divide(1000U);
    components[2] = total.divide(1000U);
    components[3] = total.divide(60U);
    components[4] = total.divide(60U);
    components[5] = total.divide(24U);
    components[6] = total.divide(7U);

    const auto smallestIndex = static_cast<std::size_t>(format.smallestUnit());
    const auto fractionalRemainder = [&components, &factors](const std::size_t unitIndex) noexcept -> int64_t {
        uint64_t result = 0;
        for (std::size_t index = 0; index < unitIndex; ++index) {
            result += static_cast<uint64_t>(components[index]) * factors[index];
        }
        return static_cast<int64_t>(result);
    };

    auto result = text::StringEditor{};
    for (const auto &definition : cUnits) {
        const auto unitIndex = static_cast<std::size_t>(definition.unit);
        if (unitIndex < smallestIndex) {
            continue;
        }
        const auto isSmallest = unitIndex == smallestIndex;
        const auto remainder = isSmallest ? fractionalRemainder(unitIndex) : 0;
        const auto hasFraction = isSmallest && format.showFractions() && remainder != 0;
        if (definition.unit == TimeDeltaUnit::Weeks) {
            if (!total.isZero() || hasFraction) {
                appendSeparator(result, format);
                if (negative) {
                    result.append(U'-');
                }
                result.append(total.magnitudeText());
                if (hasFraction) {
                    appendFraction(
                        result, remainder, static_cast<int64_t>(factors[unitIndex]), format.maximumFractionDigits());
                }
                result.append(format.valueSeparator());
                if (format.unitStyle() == TimeDeltaFormat::UnitStyle::Long) {
                    result.append(!hasFraction && total.magnitudeIsOne() ? definition.singular : definition.plural);
                } else {
                    result.append(format.usesElclUnitNames() ? definition.elclName : definition.shortName);
                }
            }
        } else {
            const auto magnitude = static_cast<int64_t>(components[unitIndex]);
            if (magnitude != 0 || hasFraction) {
                appendSeparator(result, format);
                if (negative) {
                    result.append(U'-');
                }
                result.append(text::String::fromInteger(magnitude));
                if (hasFraction) {
                    appendFraction(
                        result, remainder, static_cast<int64_t>(factors[unitIndex]), format.maximumFractionDigits());
                }
                appendUnitName(result, negative ? -magnitude : magnitude, hasFraction, definition, format);
            }
        }
        if (isSmallest) {
            break;
        }
    }
    if (result.isEmpty() && includeZero) {
        const auto zeroUnitIndex = smallestIndex > static_cast<std::size_t>(TimeDeltaUnit::Seconds)
            ? smallestIndex
            : static_cast<std::size_t>(TimeDeltaUnit::Seconds);
        const auto &zeroUnit = unitDefinition(static_cast<TimeDeltaUnit>(zeroUnitIndex));
        result.append(U'0');
        appendUnitName(result, 0, false, zeroUnit, format);
    }
    return result;
}

void appendCalendarDeltaPart(
    text::StringEditor &result,
    const int64_t value,
    const text::String &singular,
    const text::String &plural,
    const text::String &shortName,
    const text::String &elclName,
    const TimeDeltaFormat &format) {
    if (value == 0) {
        return;
    }
    appendSeparator(result, format);
    result.append(text::String::fromInteger(value));
    result.append(format.valueSeparator());
    if (format.unitStyle() == TimeDeltaFormat::UnitStyle::Long) {
        result.append(value == 1 || value == -1 ? singular : plural);
    } else {
        result.append(format.usesElclUnitNames() ? elclName : shortName);
    }
}

}
