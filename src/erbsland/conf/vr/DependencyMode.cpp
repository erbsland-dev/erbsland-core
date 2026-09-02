// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DependencyMode.hpp"

namespace erbsland::conf::vr {

using namespace text::literals;

auto DependencyMode::isValid(const bool hasSource, const bool hasTarget) const noexcept -> bool {
    if (!hasSource && !hasTarget) {
        return (_value & AllowNone) != 0;
    }
    if (hasSource && !hasTarget) {
        return (_value & AllowOnlySources) != 0;
    }
    if (!hasSource) {
        return (_value & AllowOnlyTargets) != 0;
    }
    return (_value & AllowBoth) != 0;
}

auto DependencyMode::toText() const noexcept -> const text::String & {
    for (const auto &entry : textToValueMap()) {
        if (entry.second == _value) {
            return entry.first;
        }
    }
    static const text::String undefinedText = "undefined"_el;
    return undefinedText;
}

auto DependencyMode::fromText(const text::String &text) noexcept -> DependencyMode {
    if (text.isEmpty() || text.length().toSizeT() > 20) {
        return Undefined;
    }
    for (const auto &entry : textToValueMap()) {
        if (entry.first.compare(text, text::Char::compareIdentifier) == std::strong_ordering::equal) {
            return entry.second;
        }
    }
    return Undefined;
}

auto DependencyMode::textToValueMap() noexcept -> const TextToValueMap & {
    static const TextToValueMap map = {
        {"if"_el, If},
        {"if_not"_el, IfNot},
        {"or"_el, OR},
        {"xnor"_el, XNOR},
        {"xor"_el, XOR},
        {"and"_el, AND},
    };
    return map;
}

}
