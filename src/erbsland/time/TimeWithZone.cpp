// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TimeWithZone.hpp"

#include "../text/Literals.hpp"
#include "../text/StringFormat.hpp"

namespace erbsland::time {

using namespace text::literals;

auto TimeWithZone::toString() const -> text::String {
    static const auto namedFormat = text::StringFormat{"{}[{}]"_el};
    static const auto minuteOffsetFormat = text::StringFormat{"{}{}{:02}:{:02}"_el};
    static const auto secondOffsetFormat = text::StringFormat{"{}{}{:02}:{:02}:{:02}"_el};
    if (_timeZone.isLocalTime()) {
        return _time.toString();
    }
    if (_timeZone.isNamed()) {
        return namedFormat.build(_time.toString(), _timeZone.name());
    }
    const auto offset = _timeZone.staticOffset().toSeconds().toRawValue();
    if (offset == 0) {
        return text::String::fromJoined({_time.toString(), "Z"_el});
    }
    const auto absoluteOffset = offset < 0 ? static_cast<uint64_t>(-(offset + 1)) + 1U : static_cast<uint64_t>(offset);
    const auto hours = absoluteOffset / 3600;
    const auto minutes = (absoluteOffset / 60) % 60;
    const auto seconds = absoluteOffset % 60;
    if (seconds == 0) {
        return minuteOffsetFormat.build(_time.toString(), offset < 0 ? "-"_el : "+"_el, hours, minutes);
    }
    return secondOffsetFormat.build(_time.toString(), offset < 0 ? "-"_el : "+"_el, hours, minutes, seconds);
}

}
