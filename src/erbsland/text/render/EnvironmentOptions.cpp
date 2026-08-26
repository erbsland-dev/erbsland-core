// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EnvironmentOptions.hpp"

#include "../StringCharReader.hpp"

#include "../../err/ParameterError.hpp"

#include <algorithm>

namespace erbsland::text::render {

using namespace literals;

EnvironmentOptions::EnvironmentOptions() : _escapeFormats{defaultEscapeFormats()} {
}

auto EnvironmentOptions::defaultEscapeFormats() -> const std::shared_ptr<EscapeFormats> & {
    static const auto cFormats = std::make_shared<EscapeFormats>(EscapeFormats{
        {".html"_el, EscapeFormat::Html},
        {".xml"_el, EscapeFormat::Xml},
        {".elcl"_el, EscapeFormat::Config},
        {".md"_el, EscapeFormat::Markdown},
        {".json"_el, EscapeFormat::Json},
    });
    return cFormats;
}

void EnvironmentOptions::detachEscapeFormats() {
    if (_escapeFormats.use_count() != 1) {
        _escapeFormats = std::make_shared<EscapeFormats>(*_escapeFormats);
    }
}

auto EnvironmentOptions::setAutomaticEscapingEnabled(const bool enabled) noexcept -> EnvironmentOptions & {
    _automaticEscapingEnabled = enabled;
    return *this;
}

auto EnvironmentOptions::setEscapeFormatForSuffix(const String &suffix, const EscapeFormat format)
    -> EnvironmentOptions & {
    const auto normalized = normalizedSuffix(suffix);
    detachEscapeFormats();
    const auto iterator = std::ranges::find(*_escapeFormats, normalized, &EscapeFormats::value_type::first);
    if (iterator != _escapeFormats->end()) {
        iterator->second = format;
        return *this;
    }
    if (_escapeFormats->size() >= 256U) {
        throw err::ParameterError{"There are too many layout suffix escape-format assignments."_el, "suffix"_el};
    }
    _escapeFormats->emplace_back(normalized, format);
    return *this;
}

auto EnvironmentOptions::removeEscapeFormatForSuffix(const String &suffix) -> bool {
    const auto normalized = normalizedSuffix(suffix);
    detachEscapeFormats();
    const auto iterator = std::ranges::find(*_escapeFormats, normalized, &EscapeFormats::value_type::first);
    if (iterator == _escapeFormats->end()) {
        return false;
    }
    _escapeFormats->erase(iterator);
    return true;
}

void EnvironmentOptions::clearEscapeFormats() {
    _escapeFormats = std::make_shared<EscapeFormats>();
}

auto EnvironmentOptions::escapeFormatForLayout(const String &layoutName) const noexcept -> EscapeFormat {
    if (!_automaticEscapingEnabled) {
        return EscapeFormat::None;
    }
    auto result = EscapeFormat{EscapeFormat::None};
    auto longest = unit::ByteLength{};
    for (const auto &[suffix, format] : *_escapeFormats) {
        if (suffix.length() > longest && layoutName.endsWith(suffix)) {
            result = format;
            longest = suffix.length();
        }
    }
    return result;
}

auto EnvironmentOptions::setRenderLimits(const RenderLimits &limits) noexcept -> EnvironmentOptions & {
    _renderLimits = limits;
    return *this;
}

auto EnvironmentOptions::normalizedSuffix(const String &suffix) -> String {
    if (suffix.length() < unit::ByteLength{2U} || suffix.length() > unit::ByteLength{64U}) {
        throw err::ParameterError{"A layout suffix must contain between two and 64 bytes."_el, "suffix"_el};
    }
    auto reader = StringCharReader{suffix};
    if (reader.read() != '.') {
        throw err::ParameterError{"A layout suffix must start with a period."_el, "suffix"_el};
    }
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        if (!character.isAsciiAlphanumeric() && character != '.' && character != '_' && character != '-') {
            throw err::ParameterError{
                "A layout suffix must only contain ASCII letters, digits, periods, underscores, or hyphens."_el,
                "suffix"_el};
        }
    }
    return suffix.transformed(Char::toAsciiLowercase);
}

}
