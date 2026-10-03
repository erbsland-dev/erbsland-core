// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Version.hpp"

#include "../err/ParseError.hpp"
#include "../text/String.hpp"
#include "../text/StringCharReader.hpp"
#include "../text/StringFormat.hpp"
#include "../text/StringJoins.hpp"

#include <array>
#include <limits>

namespace erbsland::unit {

using namespace err;
using namespace text;
using namespace text::literals;

auto Version::fromString(const text::String &text, VersionPart requiredPrecision) noexcept -> std::optional<Version> {
    try {
        return fromStringOrThrow(text, requiredPrecision);
    } catch (const ParseError &) {
        return std::nullopt;
    }
}

auto Version::fromStringOrThrow(const text::String &text, VersionPart requiredPrecision) -> Version {
    if (text.isEmpty()) {
        throw ParseError{"Expected a version number, but the text is empty."_el};
    }
    auto integerParseOptions = IntegerParseOptions{}
                                   .setFlags(IntegerParseFlag::IgnoreTrailingChars)
                                   .setFixedBase(IntegerBase::Decimal)
                                   .setMinimumDigits(unit::CpCount{1U});
    auto partValues = std::array<Value, 4U>{};
    auto partCount = std::size_t{0U};
    auto reader = StringCharReader{text};
    while (partCount < partValues.size()) {
        const auto startsWithZero = reader.peek() == U'0';
        const auto result = reader.parseInteger(integerParseOptions);
        if (result.status == ReadNumberStatus::Overflow) {
            throw ParseError{"A version part must not exceed 65535."_el, result.position};
        }
        if (result.status != ReadNumberStatus::Success) {
            throw ParseError{"Expected an unsigned decimal version part."_el, result.position};
        }
        if (startsWithZero && result.digitCount > unit::CpCount{1U}) {
            throw ParseError{"A version part must not have leading zeros."_el, result.position};
        }
        if (result.value > std::numeric_limits<Value>::max()) {
            throw ParseError{"A version part must not exceed 65535."_el, result.position};
        }
        partValues[partCount] = static_cast<Value>(result.value);
        ++partCount;
        if (reader.isAtEnd()) {
            break;
        }
        if (partCount == partValues.size()) {
            throw ParseError{"Unexpected characters after the fourth version part."_el, reader.position()};
        }
        if (!reader.advanceIf(U'.')) {
            throw ParseError{"Expected a '.' between version parts."_el, reader.position()};
        }
    }
    std::size_t expectedSize{};
    switch (requiredPrecision) {
    case VersionPart::Major:
        expectedSize = 1;
        break;
    case VersionPart::Minor:
        expectedSize = 2;
        break;
    case VersionPart::Revision:
        expectedSize = 3;
        break;
    default:
        expectedSize = 4;
        break;
    }
    if (partCount < expectedSize) {
        throw ParseError{
            StringFormat{"Expected at least {} version parts, but found {}."_el}.build(expectedSize, partCount)};
    }
    return Version{
        Major::fromInteger(partValues[0U]),
        Minor::fromInteger(partValues[1U]),
        Revision::fromInteger(partValues[2U]),
        BuildNumber::fromInteger(partValues[3U])};
}

auto Version::toString(const VersionPart precision) const -> String {
    const auto major = String::fromInteger(_major.toRawValue());
    if (precision == VersionPart::Major) {
        return major;
    }
    const auto minor = String::fromInteger(_minor.toRawValue());
    if (precision == VersionPart::Minor) {
        return text::join(major, String{"."}, minor);
    }
    const auto revision = String::fromInteger(_revision.toRawValue());
    if (precision == VersionPart::Revision) {
        return text::join(major, String{"."}, minor, String{"."}, revision);
    }
    return text::join(
        major, String{"."}, minor, String{"."}, revision, String{"."}, String::fromInteger(_build.toRawValue()));
}

}
