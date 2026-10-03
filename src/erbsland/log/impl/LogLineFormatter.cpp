// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LogLineFormatter.hpp"

#include "../../text/AnyString.hpp"
#include "../../text/CharSet.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringCharReader.hpp"
#include "../../text/StringJoins.hpp"
#include "../../text/StringSide.hpp"
#include "../../text/TruncateMode.hpp"
#include "../../time/IsoTimeFormat.hpp"
#include "../../time/TimeZone.hpp"

#include <memory>
#include <utility>

namespace erbsland::log::impl {

using namespace text;
using namespace text::literals;

LogLineFormatter::LogLineFormatter(LogLineFormat settings) :
    _settings{std::move(settings)}, _patternSegments{compilePattern(_settings.pattern())} {
}

auto LogLineFormatter::format(const LogEntry &entry) const -> LogLineConstPtr {
    // Convert the timestamp either in a UTC or local date/time object.
    auto timestamp = entry.timestamp().toDateTime().value_or(time::DateTime{});
    if (_settings.timestampZone() == LogTimestampZone::Local) {
        timestamp = timestamp.toTimeZone(time::TimeZone::local());
    }
    // convert the time/level/name to final strings.
    auto timeFormatFlags = time::IsoTimeFormatFlags{time::IsoTimeFormat::Extended, time::IsoTimeFormat::TimeShift};
    const auto timeText = timestamp.toIsoString(timeFormatFlags);
    const auto level = entry.level().toString(_settings.levelFormat());
    const auto name = nameText(entry.path(), _settings);
    auto message = messageText(entry.message(), _settings);
    // render the log line.
    auto result = render(timeText, level, name, message);
    if (_settings.messageTruncation() == LogMessageTruncation::TotalLineLength && !_settings.messageLimit().isZero() &&
        result->text().characterLength() > _settings.messageLimit()) {
        message = truncatedMessageForTotalLength(*result, message, _settings);
        result = render(timeText, level, name, message);
    }
    return result;
}

auto LogLineFormatter::nameText(const LogPath &path, const LogLineFormat &settings) -> String {
    const auto &value = path.value();
    if (value.isEmpty()) {
        return {};
    }
    const static auto separators = CharSet{U'/'};
    const auto first = value.findFirstOf(separators);
    const auto last = value.findLastOf(separators);
    switch (settings.nameFormat().toRawValue()) {
    case LogNameFormat::Full:
        return value;
    case LogNameFormat::Leaf:
        return last.isNoIndex() ? value : value.slice(StringSide::Back, last + unit::ByteCount::one());
    case LogNameFormat::HeadAndLeaf:
        if (first.isNoIndex() || first == last) {
            return value;
        }
        return text::join(
            value.slice(StringSide::Front, first), "/…/"_el, value.slice(StringSide::Back, last + unit::ByteCount{1U}));
    case LogNameFormat::LeftTruncated:
        if (settings.nameLimit().isZero() || value.characterLength() <= settings.nameLimit()) {
            return value;
        }
        return value.truncated(settings.nameLimit(), TruncateMode::Begin, settings.truncationMark());
    }
    return value;
}

auto LogLineFormatter::messageText(const String &message, const LogLineFormat &settings) -> String {
    switch (settings.messageTruncation().toRawValue()) {
    case LogMessageTruncation::None:
    case LogMessageTruncation::TotalLineLength:
        return message;
    case LogMessageTruncation::FirstLine: {
        const auto parts = message.splitAtFirst("\n"_el);
        if (!parts) {
            return message;
        }
        return text::join(parts->first, settings.truncationMark());
    }
    case LogMessageTruncation::CharacterCount:
        if (settings.messageLimit().isZero() || message.characterLength() <= settings.messageLimit()) {
            return message;
        }
        return message.truncated(settings.messageLimit(), TruncateMode::End, settings.truncationMark());
    }
    return message;
}

auto LogLineFormatter::truncatedMessageForTotalLength(
    const LogLine &line, const String &message, const LogLineFormat &settings) -> String {
    auto occurrences = std::size_t{};
    for (const auto &segment : line.segments()) {
        if (segment.part == LogLinePart::Message) {
            ++occurrences;
        }
    }
    if (occurrences == 0U) {
        return message;
    }
    const auto messageLength = message.characterLength();
    const auto fixedLength = line.text().characterLength() - messageLength * occurrences;
    const auto maximum = settings.messageLimit();
    if (maximum <= fixedLength) {
        return {};
    }
    return message.truncated((maximum - fixedLength) / occurrences, TruncateMode::End, settings.truncationMark());
}

auto LogLineFormatter::compilePattern(const String &pattern) -> std::vector<LogLineSegment> {
    auto segments = std::vector<LogLineSegment>{};
    const auto appendLiteral = [&segments](String literal) -> void {
        if (!literal.isEmpty()) {
            segments.push_back(LogLineSegment{LogLinePart::Literal, std::move(literal)});
        }
    };
    static const auto cBraces = CharSet{U'{', U'}'};
    static const auto cCloseBrace = CharSet{U'}'};
    auto reader = StringCharReader{pattern};
    reader.startCapture();
    while (!reader.isAtEnd()) {
        reader.advanceUntil(cBraces);
        reader.appendCaptureToBuffer();
        if (reader.isAtEnd()) {
            break;
        }
        const auto brace = reader.read();
        if (brace == U'}') {
            reader.advanceIf(U'}');
            reader.appendToBuffer(U'}');
            reader.startCapture();
            continue;
        }
        if (reader.advanceIf(U'{')) {
            reader.appendToBuffer(U'{');
            reader.startCapture();
            continue;
        }
        appendLiteral(reader.takeBuffer().toString());
        reader.startCapture();
        reader.advanceUntil(cCloseBrace);
        const auto key = reader.takeCapture().toString();
        if (!reader.advanceIf(U'}')) {
            appendLiteral(text::join("{"_el, key));
            break;
        }
        if (const auto part = LogLinePart::fromString(key); part.has_value()) {
            segments.push_back({*part, {}});
        } else {
            segments.push_back({LogLinePart::Literal, text::join("{"_el, key, "}"_el)});
        }
        reader.startCapture();
    }
    appendLiteral(reader.takeBuffer().toString());
    return segments;
}

auto LogLineFormatter::render(const String &time, const String &level, const String &name, const String &message) const
    -> LogLineConstPtr {
    auto segments = _patternSegments;
    for (auto &segment : segments) {
        switch (segment.part.toRawValue()) {
        case LogLinePart::Time:
            segment.text = time;
            break;
        case LogLinePart::Level:
            segment.text = level;
            break;
        case LogLinePart::Name:
            segment.text = name;
            break;
        case LogLinePart::Message:
            segment.text = message;
            break;
        case LogLinePart::Literal:
        case LogLinePart::_Count:
            break;
        }
    }
    return std::make_shared<LogLine>(std::move(segments));
}

}
