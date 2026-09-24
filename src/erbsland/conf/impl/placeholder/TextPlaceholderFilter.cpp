// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TextPlaceholderFilter.hpp"

#include "TextPlaceholderParameters.hpp"

#include "../../../text/Char.hpp"
#include "../../../text/Literals.hpp"
#include "../../ConfError.hpp"

namespace erbsland::conf::impl::placeholder {

using namespace text::literals;

auto TextPlaceholderFilter::filterNames() const -> text::StringList {
    return text::StringList{
        "safe"_el,
        "trim"_el,
        "slice"_el,
        "remove"_el,
        "replace"_el,
        "escape"_el,
        "if"_el,
        "error_if"_el,
        "default"_el,
        "required"_el,
        "lower"_el,
        "upper"_el};
}

auto TextPlaceholderFilter::apply(
    const text::String &filterName, const text::String &parameter, const text::String &value) -> text::String {
    if (filterName == "default"_el) {
        return value.isEmpty() ? parameter : value;
    }
    if (filterName == "required"_el) {
        if (!parameter.isEmpty()) {
            throw ConfError{ConfErrorCategory::Syntax, "The 'required' filter accepts no parameter."_el};
        }
        if (value.isEmpty()) {
            throw ConfError{ConfErrorCategory::Validation, "A required placeholder value is empty."_el};
        }
        return value;
    }
    if (filterName == "lower"_el || filterName == "upper"_el) {
        return applyCase(filterName, parameter, value);
    }
    const auto parameters = TextPlaceholderParameters::parse(parameter);
    if (filterName == "safe"_el) {
        return parameters.applySafe(value);
    }
    if (filterName == "trim"_el) {
        return parameters.applyTrim(value);
    }
    if (filterName == "slice"_el) {
        return parameters.applySlice(value);
    }
    if (filterName == "remove"_el) {
        return parameters.applyRemove(value);
    }
    if (filterName == "replace"_el) {
        return parameters.applyReplace(value);
    }
    if (filterName == "escape"_el) {
        return parameters.applyEscape(value);
    }
    if (filterName == "if"_el) {
        return parameters.applyIf(value);
    }
    if (filterName == "error_if"_el) {
        return parameters.applyErrorIf(value);
    }
    throw ConfError{ConfErrorCategory::Unsupported, "Unknown built-in placeholder filter."_el};
}

auto TextPlaceholderFilter::applyCase(
    const text::String &filterName, const text::String &parameter, const text::String &value) -> text::String {
    const auto mode = parameter.transformed(text::Char::toAsciiLowercase);
    if (!mode.isEmpty() && mode != "ascii"_el && mode != "unicode"_el) {
        throw ConfError{ConfErrorCategory::Syntax, "The case-conversion mode must be 'ascii' or 'unicode'."_el};
    }
    if (filterName == "lower"_el) {
        if (mode == "ascii"_el) {
            return value.transformed(
                [](const text::Char character) noexcept -> text::Char { return character.toAsciiLowercase(); });
        }
        return value.transformed(
            [](const text::Char character) noexcept -> text::Char { return character.toLowercase(); });
    }
    if (mode == "ascii"_el) {
        return value.transformed(
            [](const text::Char character) noexcept -> text::Char { return character.toAsciiUppercase(); });
    }
    return value.transformed([](const text::Char character) noexcept -> text::Char { return character.toUppercase(); });
}

}
