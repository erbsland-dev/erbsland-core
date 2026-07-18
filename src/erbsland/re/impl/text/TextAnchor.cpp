// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TextAnchor.hpp"

#include "../../../err/ParameterError.hpp"
#include "../../../text/Literals.hpp"

namespace erbsland::re::impl {

using namespace text::literals;

auto TextAnchor::toString() const -> text::String {
    for (const auto &[value, name] : valueToNameList()) {
        if (value == _value) {
            return name;
        }
    }
    return {};
}

auto TextAnchor::fromString(const text::String &str) -> TextAnchor {
    for (const auto &[value, name] : valueToNameList()) {
        if (name.compare(str, text::Char::compareCaseFolded) == std::strong_ordering::equal) {
            return value;
        }
    }
    throw err::ParameterError{"Invalid text anchor name."_el, "str"_el};
}

auto TextAnchor::valueToNameList() noexcept -> const ValueToNameList & {
    using namespace text::literals;
    static const ValueToNameList list = {
        {Start, "Start"_el},
        {End, "End"_el},
        {LineStart, "LineStart"_el},
        {LineEnd, "LineEnd"_el},
        {UnicodeWordBoundary, "UnicodeWordBoundary"_el},
        {AsciiWordBoundary, "AsciiWordBoundary"_el},
        {NonUnicodeWordBoundary, "NonUnicodeWordBoundary"_el},
        {NonAsciiWordBoundary, "NonAsciiWordBoundary"_el},
        // Short aliases for the Assembler.
        {UnicodeWordBoundary, "UWordBoundary"_el},
        {AsciiWordBoundary, "AWordBoundary"_el},
        {NonUnicodeWordBoundary, "NUWordBoundary"_el},
        {NonAsciiWordBoundary, "NAWordBoundary"_el},
    };
    return list;
}

}
