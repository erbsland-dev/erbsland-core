// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ConfigEscapeFormatter.hpp"

#include "../IntegerFormat.hpp"
#include "../Literals.hpp"

#include <algorithm>
#include <bit>

namespace erbsland::text::impl {

using namespace text::literals;

auto ConfigEscapeFormatter::unicodeEscapeFormat() noexcept -> IntegerFormat {
    return IntegerFormat{IntegerBase::Hexadecimal}.setLetterCase(LetterCase::Lowercase);
}

auto ConfigEscapeFormatter::hexadecimalDigitCount(const Char character) noexcept -> std::size_t {
    const auto bits = static_cast<std::size_t>(std::bit_width(static_cast<uint32_t>(character.toRawValue())));
    return std::max<std::size_t>(1U, (bits + 3U) / 4U);
}

auto ConfigEscapeFormatter::needsEscape(const Char character, const EscapeAmount amount) const noexcept -> bool {
    const static auto regularRequiredSet = []() -> CharSet {
        auto result = CharSet::from(UnicodeCategory::Control);
        result.add(CharSet{U'"', U'\\', U'$', U'\n', U'\r'});
        result.add(Char::replacement());
        return result;
    }();
    const static auto testRequiredSet = []() -> CharSet {
        auto result = CharSet{U'\\', U'\"', U'.', U':', U'=', U'$'};
        result.add(CharRange{0x00U, 0x1fU});
        result.add(CharRange{0x7fU, 0x10ffffU});
        return result;
    }();
    const auto &activeRequiredSet = _format == Format::Regular ? regularRequiredSet : testRequiredSet;
    switch (amount.toRawValue()) {
    case EscapeAmount::Nothing:
        return false;
    case EscapeAmount::Required:
        return activeRequiredSet.contains(character);
    case EscapeAmount::Balanced:
        return activeRequiredSet.contains(character) || character.isControlOrFormat();
    case EscapeAmount::NonAscii:
        return activeRequiredSet.contains(character) || character.isControlOrFormat() || !character.isAscii();
    case EscapeAmount::Everything:
        return true;
    }
    return false;
}

void ConfigEscapeFormatter::escape(const Char character, AnyStringBuilder &builder) const {
    if (_format == Format::Regular) {
        // the short forms are only used for regular escape format.
        switch (character.toRawValue()) {
        case U'"':
            builder.append("\\\""_el);
            return;
        case U'\\':
            builder.append("\\\\"_el);
            return;
        case U'$':
            builder.append("\\$"_el);
            return;
        case U'\n':
            builder.append("\\n"_el);
            return;
        case U'\r':
            builder.append("\\r"_el);
            return;
        case U'\t':
            builder.append("\\t"_el);
            return;
        default:
            break;
        }
    }
    builder.append("\\u{"_el);
    builder.appendInteger(character.toRawValue(), unicodeEscapeFormat());
    builder.append(U'}');
}

auto ConfigEscapeFormatter::escapeSize(
    const Char character, [[maybe_unused]] const StringKind stringKind) const noexcept -> std::size_t {
    switch (character.toRawValue()) {
    case U'"':
    case U'\\':
    case U'$':
    case U'\n':
    case U'\r':
    case U'\t':
        return 2U;
    default:
        break;
    }
    return hexadecimalDigitCount(character.isValidUnicode() ? character : Char::replacement()) + 4U;
}

auto ConfigEscapeFormatter::regularInstance() noexcept -> const EscapeFormatterPtr & {
    static EscapeFormatterPtr result = std::make_shared<ConfigEscapeFormatter>(Format::Regular);
    return result;
}

auto ConfigEscapeFormatter::testInstance() noexcept -> const EscapeFormatterPtr & {
    static EscapeFormatterPtr result = std::make_shared<ConfigEscapeFormatter>(Format::Test);
    return result;
}

}
