// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LogLineFormat.hpp"

#include "line/LogLinePart.hpp"

#include "../err/ParameterFailure.hpp"
#include "../text/AnyString.hpp"
#include "../text/Literals.hpp"
#include "../text/StringCharReader.hpp"

#include <utility>

namespace erbsland::log {

using namespace text;
using namespace text::literals;

LogLineFormat::LogLineFormat() : _pattern{"{time} {level} - {message}"_el}, _truncationMark{"…"_el} {
}

auto LogLineFormat::setPattern(String value) -> LogLineFormat & {
    if (!isValidPattern(value)) {
        throw err::ParameterFailure{"Invalid log-line pattern."_el, "value"_el};
    }
    _pattern = std::move(value);
    return *this;
}

auto LogLineFormat::isValidPattern(const String &value) noexcept -> bool {
    if (value.isEmpty()) {
        return false;
    }
    auto reader = StringCharReader{value};
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        if (character == U'}') {
            if (!reader.advanceIf(U'}')) {
                return false;
            }
            continue;
        }
        if (character != U'{' || reader.advanceIf(U'{')) {
            continue;
        }
        reader.startCapture();
        reader.advanceUntil(CharSet{U'}'});
        const auto key = reader.takeCapture().toString();
        if (!reader.advanceIf(U'}')) {
            return false;
        }
        if (!LogLinePart::fromString(key).has_value()) {
            return false;
        }
    }
    return true;
}

auto LogLineFormat::setTimestampZone(const LogTimestampZone value) noexcept -> LogLineFormat & {
    _timestampZone = value;
    return *this;
}

auto LogLineFormat::setLevelFormat(const LogLevelFormat value) noexcept -> LogLineFormat & {
    _levelFormat = value;
    return *this;
}

auto LogLineFormat::setNameFormat(const LogNameFormat value) noexcept -> LogLineFormat & {
    _nameFormat = value;
    return *this;
}

auto LogLineFormat::setNameLimit(const unit::CpCount value) noexcept -> LogLineFormat & {
    _nameLimit = value;
    return *this;
}

auto LogLineFormat::setMessageTruncation(const LogMessageTruncation value) noexcept -> LogLineFormat & {
    _messageTruncation = value;
    return *this;
}

auto LogLineFormat::setMessageLimit(const unit::CpCount value) noexcept -> LogLineFormat & {
    _messageLimit = value;
    return *this;
}

auto LogLineFormat::setTruncationMark(String value) noexcept -> LogLineFormat & {
    _truncationMark = std::move(value);
    return *this;
}

}
