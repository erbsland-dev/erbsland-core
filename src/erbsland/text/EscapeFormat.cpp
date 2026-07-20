// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EscapeFormat.hpp"

#include "Literals.hpp"
#include "String.hpp"

#include "impl/ThrowHelper.hpp"

#include <algorithm>
#include <ranges>

namespace erbsland::text {

using namespace text::literals;

const EscapeFormat::ValueToTextArray EscapeFormat::_valueToTextMap = {
    {
        {None, "none"_el},
        {Html, "html"_el},
        {Json, "json"_el},
        {Cpp, "cpp"_el},
        {Xml, "xml"_el},
        {RegEx, "regex"_el},
        {Display, "display"_el},
        {Config, "config"_el},
        {ConfigTest, "config_test"_el},
    },
};

auto EscapeFormat::toString() const -> String {
    const auto it =
        std::ranges::find_if(_valueToTextMap, [this](const auto &pair) -> bool { return pair.first == _value; });
    if (it != _valueToTextMap.end()) {
        return it->second;
    }
    return "none"_el;
}

auto EscapeFormat::fromString(const String &text) noexcept -> std::optional<EscapeFormat> {
    const auto it =
        std::ranges::find_if(_valueToTextMap, [&](const auto &pair) -> bool { return text == pair.second; });
    if (it != _valueToTextMap.end()) {
        return it->first;
    }
    return std::nullopt;
}

auto EscapeFormat::fromStringOrThrow(const String &text) -> EscapeFormat {
    if (const auto result = fromString(text); result.has_value()) {
        return result.value();
    }
    impl::throwParseError("Unsupported escape format");
}

}
