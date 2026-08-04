// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TimeDeltaFormatter.hpp"

#include "../../math/BigInteger.hpp"
#include "../../text/Literals.hpp"

#include <array>
#include <cstddef>

namespace erbsland::time::impl {

using namespace text::literals;

constexpr auto cTimeDeltaUnitDefinitions = std::array{
    TimeDeltaUnitDefinition{TimeDeltaUnit::Weeks, 604800000000000LL, "w"_el, "w"_el, "week"_el, "weeks"_el},
    TimeDeltaUnitDefinition{TimeDeltaUnit::Days, 86400000000000LL, "d"_el, "d"_el, "day"_el, "days"_el},
    TimeDeltaUnitDefinition{TimeDeltaUnit::Hours, 3600000000000LL, "h"_el, "h"_el, "hour"_el, "hours"_el},
    TimeDeltaUnitDefinition{TimeDeltaUnit::Minutes, 60000000000LL, "min"_el, "m"_el, "minute"_el, "minutes"_el},
    TimeDeltaUnitDefinition{TimeDeltaUnit::Seconds, 1000000000LL, "s"_el, "s"_el, "second"_el, "seconds"_el},
    TimeDeltaUnitDefinition{
        TimeDeltaUnit::Milliseconds, 1000000LL, "ms"_el, "ms"_el, "millisecond"_el, "milliseconds"_el},
    TimeDeltaUnitDefinition{TimeDeltaUnit::Microseconds, 1000LL, "us"_el, "us"_el, "microsecond"_el, "microseconds"_el},
    TimeDeltaUnitDefinition{TimeDeltaUnit::Nanoseconds, 1LL, "ns"_el, "ns"_el, "nanosecond"_el, "nanoseconds"_el},
};

auto timeDeltaUnitDefinition(const TimeDeltaUnit unit) noexcept -> const TimeDeltaUnitDefinition & {
    for (const auto &definition : cTimeDeltaUnitDefinitions) {
        if (definition.unit == unit) {
            return definition;
        }
    }
    return cTimeDeltaUnitDefinitions.back();
}

void appendTimeDeltaUnitSeparator(text::StringEditor &result, const TimeDeltaFormat &format) {
    if (!result.isEmpty()) {
        result.append(format.unitSeparator());
    }
}

void appendTimeDeltaUnitName(
    text::StringEditor &result,
    const int64_t value,
    const bool hasFraction,
    const TimeDeltaUnitDefinition &definition,
    const TimeDeltaFormat &format) {
    result.append(format.valueSeparator());
    if (format.unitStyle() == TimeDeltaFormat::UnitStyle::Long) {
        result.append(!hasFraction && (value == 1 || value == -1) ? definition.singular : definition.plural);
    } else {
        result.append(format.usesElclUnitNames() ? definition.elclName : definition.shortName);
    }
}

void appendTimeDeltaFraction(
    text::StringEditor &result, int64_t remainder, const int64_t factor, const uint8_t maximumDigits) {
    static const auto cZeroCharacters = text::CharSet{U'0'};
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
    fraction.trim(cZeroCharacters, text::StringSide::Back);
    if (!fraction.isEmpty()) {
        result.append(U'.');
        result.append(fraction);
    }
}

auto formatTimeDelta(const Nanoseconds value, const TimeDeltaFormat &format, const bool includeZero) -> text::String {
    auto result = text::StringEditor{};
    auto remaining = value.toRawValue();
    const auto &smallest = timeDeltaUnitDefinition(format.smallestUnit());
    for (const auto &definition : cTimeDeltaUnitDefinitions) {
        if (definition.factor < smallest.factor) {
            continue;
        }
        const auto component = remaining / definition.factor;
        remaining %= definition.factor;
        const auto isSmallest = definition.unit == smallest.unit;
        const auto hasFraction = isSmallest && format.showFractions() && remaining != 0;
        if (component != 0 || hasFraction) {
            appendTimeDeltaUnitSeparator(result, format);
            if (component == 0 && remaining < 0) {
                result.append(U'-');
            }
            result.append(text::String::fromInteger(component));
            if (isSmallest && format.showFractions()) {
                appendTimeDeltaFraction(result, remaining, definition.factor, format.maximumFractionDigits());
            }
            appendTimeDeltaUnitName(result, component, hasFraction, definition, format);
        }
        if (isSmallest) {
            break;
        }
    }
    if (result.isEmpty() && includeZero) {
        const auto &zeroUnit = timeDeltaUnitDefinition(
            smallest.factor > timeDeltaUnitDefinition(TimeDeltaUnit::Seconds).factor ? smallest.unit
                                                                                     : TimeDeltaUnit::Seconds);
        result.append(text::String::fromInteger(0));
        appendTimeDeltaUnitName(result, 0, false, zeroUnit, format);
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
    constexpr auto cFactors = std::array<uint64_t, 8>{
        1ULL,
        1'000ULL,
        1'000'000ULL,
        1'000'000'000ULL,
        60'000'000'000ULL,
        3'600'000'000'000ULL,
        86'400'000'000'000ULL,
        604'800'000'000'000ULL};
    auto total = math::BigInteger{};
    total += math::BigInteger{nanoseconds.toRawValue()} * math::BigInteger{cFactors[0]};
    total += math::BigInteger{microseconds.toRawValue()} * math::BigInteger{cFactors[1]};
    total += math::BigInteger{milliseconds.toRawValue()} * math::BigInteger{cFactors[2]};
    total += math::BigInteger{seconds.toRawValue()} * math::BigInteger{cFactors[3]};
    total += math::BigInteger{minutes.toRawValue()} * math::BigInteger{cFactors[4]};
    total += math::BigInteger{hours.toRawValue()} * math::BigInteger{cFactors[5]};
    total += math::BigInteger{days.toRawValue()} * math::BigInteger{cFactors[6]};
    total += math::BigInteger{weeks.toRawValue()} * math::BigInteger{cFactors[7]};

    const auto negative = total.isNegative();
    auto remainingMagnitude = total.magnitude();
    auto components = std::array<uint32_t, 7>{};
    components[0] = remainingMagnitude.divideGetRemainder(math::BigUnsignedInteger{1000U}).cast<uint32_t>();
    components[1] = remainingMagnitude.divideGetRemainder(math::BigUnsignedInteger{1000U}).cast<uint32_t>();
    components[2] = remainingMagnitude.divideGetRemainder(math::BigUnsignedInteger{1000U}).cast<uint32_t>();
    components[3] = remainingMagnitude.divideGetRemainder(math::BigUnsignedInteger{60U}).cast<uint32_t>();
    components[4] = remainingMagnitude.divideGetRemainder(math::BigUnsignedInteger{60U}).cast<uint32_t>();
    components[5] = remainingMagnitude.divideGetRemainder(math::BigUnsignedInteger{24U}).cast<uint32_t>();
    components[6] = remainingMagnitude.divideGetRemainder(math::BigUnsignedInteger{7U}).cast<uint32_t>();

    const auto smallestIndex = static_cast<std::size_t>(format.smallestUnit());
    const auto fractionalRemainder = [&components, &cFactors](const std::size_t unitIndex) noexcept -> int64_t {
        uint64_t result = 0;
        for (std::size_t index = 0; index < unitIndex; ++index) {
            result += static_cast<uint64_t>(components[index]) * cFactors[index];
        }
        return static_cast<int64_t>(result);
    };

    auto result = text::StringEditor{};
    for (const auto &definition : cTimeDeltaUnitDefinitions) {
        const auto unitIndex = static_cast<std::size_t>(definition.unit);
        if (unitIndex < smallestIndex) {
            continue;
        }
        const auto isSmallest = unitIndex == smallestIndex;
        const auto remainder = isSmallest ? fractionalRemainder(unitIndex) : 0;
        const auto hasFraction = isSmallest && format.showFractions() && remainder != 0;
        if (definition.unit == TimeDeltaUnit::Weeks) {
            if (!remainingMagnitude.isZero() || hasFraction) {
                appendTimeDeltaUnitSeparator(result, format);
                if (negative) {
                    result.append(U'-');
                }
                result.append(remainingMagnitude.toString());
                if (hasFraction) {
                    appendTimeDeltaFraction(
                        result, remainder, static_cast<int64_t>(cFactors[unitIndex]), format.maximumFractionDigits());
                }
                appendTimeDeltaUnitName(result, remainingMagnitude.isOne() ? 1 : 2, hasFraction, definition, format);
            }
        } else {
            const auto magnitude = static_cast<int64_t>(components[unitIndex]);
            if (magnitude != 0 || hasFraction) {
                appendTimeDeltaUnitSeparator(result, format);
                if (negative) {
                    result.append(U'-');
                }
                result.append(text::String::fromInteger(magnitude));
                if (hasFraction) {
                    appendTimeDeltaFraction(
                        result, remainder, static_cast<int64_t>(cFactors[unitIndex]), format.maximumFractionDigits());
                }
                appendTimeDeltaUnitName(result, negative ? -magnitude : magnitude, hasFraction, definition, format);
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
        const auto &zeroUnit = timeDeltaUnitDefinition(static_cast<TimeDeltaUnit>(zeroUnitIndex));
        result.append(U'0');
        appendTimeDeltaUnitName(result, 0, false, zeroUnit, format);
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
    appendTimeDeltaUnitSeparator(result, format);
    result.append(text::String::fromInteger(value));
    result.append(format.valueSeparator());
    if (format.unitStyle() == TimeDeltaFormat::UnitStyle::Long) {
        result.append(value == 1 || value == -1 ? singular : plural);
    } else {
        result.append(format.usesElclUnitNames() ? elclName : shortName);
    }
}

}
