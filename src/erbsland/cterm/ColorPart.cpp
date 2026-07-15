// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ColorPart.hpp"

#include "impl/BlockTextUtil.hpp"

#include "../err/ParameterError.hpp"
#include "../text/StringConverter.hpp"

#include <algorithm>
#include <ranges>

namespace erbsland::cterm {

using namespace text::literals;

auto ColorBase::tableEntry() const noexcept -> const TableEntry & {
    return colorTable()[static_cast<std::size_t>(_value)];
}

auto ColorBase::colorTable() noexcept -> const ColorTable & {
    static const auto table = ColorTable{{
        {Value::Black, 0, "black"_el},
        {Value::Red, 1, "red"_el},
        {Value::Green, 2, "green"_el},
        {Value::Yellow, 3, "yellow"_el},
        {Value::Blue, 4, "blue"_el},
        {Value::Magenta, 5, "magenta"_el},
        {Value::Cyan, 6, "cyan"_el},
        {Value::White, 7, "white"_el},
        {Value::BrightBlack, 60, "bright_black"_el},
        {Value::BrightRed, 61, "bright_red"_el},
        {Value::BrightGreen, 62, "bright_green"_el},
        {Value::BrightYellow, 63, "bright_yellow"_el},
        {Value::BrightBlue, 64, "bright_blue"_el},
        {Value::BrightMagenta, 65, "bright_magenta"_el},
        {Value::BrightCyan, 66, "bright_cyan"_el},
        {Value::BrightWhite, 67, "bright_white"_el},
        {Value::Default, 9, "default"_el},
        {Value::Inherited, 9, "inherited"_el},
    }};
    return table;
}

auto ColorBase::toString() const -> text::StringView {
    return tableEntry().name;
}

auto ColorBase::enumFromString(const text::StringView &str) -> Value {
    const auto normalizedIdentifier = str.trimmed().transformed(text::Char::toIdentifierNormalized);
    const auto &table = colorTable();
    auto it =
        std::ranges::find_if(table, [&](const auto &entry) -> bool { return normalizedIdentifier == entry.name; });
    if (it == table.end()) {
        throw err::ParameterError{"Unknown color."_el, "str"_el};
    }
    return it->value;
}

auto ColorBase::brighterEnum(Value value) -> Value {
    switch (value) {
    case Value::Black:
        return Value::BrightBlack;
    case Value::Red:
        return Value::BrightRed;
    case Value::Green:
        return Value::BrightGreen;
    case Value::Yellow:
        return Value::BrightYellow;
    case Value::Blue:
        return Value::BrightBlue;
    case Value::Magenta:
        return Value::BrightMagenta;
    case Value::Cyan:
        return Value::BrightCyan;
    case Value::White:
        return Value::BrightWhite;
    default:
        return value;
    }
}

}
