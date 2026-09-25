// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ReplacerOptions.hpp"

#include "../Literals.hpp"
#include "../StringSide.hpp"

#include "../../err/LogicError.hpp"
#include "../../err/ParameterError.hpp"
#include "../../unit/CpIndex.hpp"

#include <utility>

namespace erbsland::text::placeholder {

using namespace literals;

ReplacerOptions::ReplacerOptions() :
    _frameBegin{"${"_el}, _frameEnd{"}"_el}, _filterSeparator{"|"_el}, _nameSeparator{":"_el} {
}

void ReplacerOptions::verifyLength(const String &value, const String &name, const bool allowEmpty) {
    const auto length = value.characterLength().toSizeT();
    if ((!allowEmpty && length == 0U) || length > 16U) {
        throw err::ParameterError{
            "A placeholder delimiter must contain 1–16 code points, or be empty when optional."_el, name};
    }
}

auto ReplacerOptions::setFrame(String begin, String end) -> ReplacerOptions & {
    verifyLength(begin, "begin"_el, false);
    verifyLength(end, "end"_el, false);
    _frameBegin = std::move(begin);
    _frameEnd = std::move(end);
    return *this;
}

auto ReplacerOptions::setFilterSeparator(String separator) -> ReplacerOptions & {
    verifyLength(separator, "filterSeparator"_el, true);
    _filterSeparator = std::move(separator);
    return *this;
}

auto ReplacerOptions::setNameSeparator(String separator) -> ReplacerOptions & {
    verifyLength(separator, "nameSeparator"_el, true);
    _nameSeparator = std::move(separator);
    return *this;
}

auto ReplacerOptions::setEscapeMode(const EscapeMode mode) noexcept -> ReplacerOptions & {
    _escapeMode = mode;
    return *this;
}

auto ReplacerOptions::hasDoubledStart(const String &value) noexcept -> bool {
    return !value.isEmpty() && value.charAt(StringSide::Front) == value.charAt(unit::CpIndex{1U});
}

void ReplacerOptions::validate() const {
    if (_escapeMode == EscapeMode::Double &&
        (hasDoubledStart(_frameBegin) || hasDoubledStart(_frameEnd) || hasDoubledStart(_filterSeparator) ||
            hasDoubledStart(_nameSeparator))) {
        throw err::LogicError{"Double escape mode cannot use a delimiter with a doubled first code point."_el};
    }
}

}
