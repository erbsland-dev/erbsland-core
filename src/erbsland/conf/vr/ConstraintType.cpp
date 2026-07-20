// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ConstraintType.hpp"

#include <cassert>

namespace erbsland::conf::vr {

using namespace text::literals;

ConstraintType::ValueToTextMap ConstraintType::_valueToTextMap = {
    ValueToTextEntry(Undefined, "Undefined"_el),
    ValueToTextEntry(Chars, "Chars"_el),
    ValueToTextEntry(Contains, "Contains"_el),
    ValueToTextEntry(Ends, "Ends"_el),
    ValueToTextEntry(Equals, "Equals"_el),
    ValueToTextEntry(In, "In"_el),
    ValueToTextEntry(ConfKey, "ConfKey"_el),
    ValueToTextEntry(Matches, "Matches"_el),
    ValueToTextEntry(Maximum, "Maximum"_el),
    ValueToTextEntry(MaximumVersion, "MaximumVersion"_el),
    ValueToTextEntry(Minimum, "Minimum"_el),
    ValueToTextEntry(MinimumVersion, "MinimumVersion"_el),
    ValueToTextEntry(Multiple, "Multiple"_el),
    ValueToTextEntry(Starts, "Starts"_el),
    ValueToTextEntry(ConfVersion, "ConfVersion"_el),
};

ConstraintType::TextToValueMap ConstraintType::_textToValueMap = {
    {"chars"_el, Chars},
    {"contains"_el, Contains},
    {"equals"_el, Equals},
    {"ends"_el, Ends},
    {"in"_el, In},
    {"key"_el, ConfKey},
    {"matches"_el, Matches},
    {"maximum"_el, Maximum},
    {"maximum_version"_el, MaximumVersion},
    {"minimum"_el, Minimum},
    {"minimum_version"_el, MinimumVersion},
    {"multiple"_el, Multiple},
    {"starts"_el, Starts},
    {"version"_el, ConfVersion},
};

auto ConstraintType::toText() const noexcept -> const text::String & {
    auto it = std::ranges::find_if(
        _valueToTextMap, [this](const ValueToTextEntry &entry) -> bool { return entry.value == _value; });
    assert(it != _valueToTextMap.end());
    return it->text;
}

auto ConstraintType::fromText(const text::String &text) noexcept -> ConstraintType {
    if (text.isEmpty() || text.length().toSizeT() > 20) {
        return Undefined;
    }
    for (const auto &[name, value] : _textToValueMap) {
        if (name.compare(text, text::Char::compareIdentifier) == std::strong_ordering::equal) {
            return value;
        }
    }
    return Undefined;
}

}
