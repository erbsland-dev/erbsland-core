// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TextFilter.hpp"

#include "ReplacerError.hpp"

#include "impl/TextParameters.hpp"

#include "../Char.hpp"
#include "../Literals.hpp"

namespace erbsland::text::placeholder {

using namespace literals;

auto TextFilter::filterNames() const -> StringList {
    return StringList{
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

auto TextFilter::validate(const String &filterName, const String &parameter) -> bool {
    try {
        if (filterName == "default"_el) {
            return true;
        }
        if (filterName == "required"_el) {
            return parameter.isEmpty();
        }
        if (filterName == "lower"_el || filterName == "upper"_el) {
            const auto mode = parameter.transformed(Char::toAsciiLowercase);
            return mode.isEmpty() || mode == "ascii"_el || mode == "unicode"_el;
        }
        impl::TextParameters::parse(parameter).validateFor(filterName);
        return true;
    } catch (const ReplacerError &) {
        return false;
    }
}

auto TextFilter::apply(const String &filterName, const String &parameter, const String &value) -> String {
    if (filterName == "default"_el) {
        return value.isEmpty() ? parameter : value;
    }
    if (filterName == "required"_el) {
        if (!parameter.isEmpty()) {
            throw ReplacerError{ReplacerErrorCategory::Syntax, "The 'required' filter accepts no parameter."_el};
        }
        if (value.isEmpty()) {
            throw ReplacerError{ReplacerErrorCategory::Validation, "A required placeholder value is empty."_el};
        }
        return value;
    }
    if (filterName == "lower"_el || filterName == "upper"_el) {
        return applyCase(filterName, parameter, value);
    }
    const auto parameters = impl::TextParameters::parse(parameter);
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
    throw ReplacerError{ReplacerErrorCategory::Unsupported, "Unknown built-in placeholder filter."_el};
}

auto TextFilter::applyCase(const String &filterName, const String &parameter, const String &value) -> String {
    const auto mode = parameter.transformed(Char::toAsciiLowercase);
    if (!mode.isEmpty() && mode != "ascii"_el && mode != "unicode"_el) {
        throw ReplacerError{ReplacerErrorCategory::Syntax, "The case-conversion mode must be 'ascii' or 'unicode'."_el};
    }
    if (filterName == "lower"_el) {
        if (mode == "ascii"_el) {
            return value.transformed(
                [](const Char character) noexcept -> Char { return character.toAsciiLowercase(); });
        }
        return value.transformed([](const Char character) noexcept -> Char { return character.toLowercase(); });
    }
    if (mode == "ascii"_el) {
        return value.transformed([](const Char character) noexcept -> Char { return character.toAsciiUppercase(); });
    }
    return value.transformed([](const Char character) noexcept -> Char { return character.toUppercase(); });
}

}
