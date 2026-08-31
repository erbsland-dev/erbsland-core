// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LogLineFormatter.hpp"

#include "../../text/AnyString.hpp"
#include "../../text/CharSet.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringCharReader.hpp"
#include "../../text/StringSide.hpp"
#include "../../text/TruncateMode.hpp"
#include "../../time/IsoTimeFormat.hpp"
#include "../../time/TimeZone.hpp"

namespace erbsland::log::impl {

using namespace text::literals;

auto LogLineFormatter::format() const -> LogLineConstPtr {
    const auto timestamp = _settings.timestampZone() == LogTimestampZone::Local
        ? _entry.timestamp().toTimeZone(time::TimeZone::local())
        : _entry.timestamp();
    const auto timeText =
        timestamp.toIsoString(time::IsoTimeFormatFlags{time::IsoTimeFormat::Extended, time::IsoTimeFormat::TimeShift});
    const auto level = _entry.level().toString(_settings.levelFormat());
    const auto name = nameText(_entry.path(), _settings);
    auto message = messageText(_entry.message(), _settings);
    auto result = render(_settings, timeText, level, name, message);
    if (_settings.messageTruncation() == LogMessageTruncation::TotalLineLength && !_settings.messageLimit().isZero() &&
        result->text().characterLength() > _settings.messageLimit()) {
        message = truncatedMessageForTotalLength(*result, message, _settings);
        result = render(_settings, timeText, level, name, message);
    }
    return result;
}

auto LogLineFormatter::nameText(const LogPath &path, const LogLineFormat &settings) -> text::String {
    const auto &value = path.value();
    if (value.isEmpty()) {
        return {};
    }
    const static auto separators = text::CharSet{U'/'};
    const auto first = value.findFirstOf(separators);
    const auto last = value.findLastOf(separators);
    switch (settings.nameFormat().toRawValue()) {
    case LogNameFormat::Full:
        return value;
    case LogNameFormat::Leaf:
        return last.isNoIndex() ? value : value.slice(text::StringSide::Back, last + unit::ByteLength{1U});
    case LogNameFormat::HeadAndLeaf:
        if (first.isNoIndex() || first == last) {
            return value;
        }
        return text::String::fromJoined(
            {value.slice(text::StringSide::Front, first),
                "/…/"_el,
                value.slice(text::StringSide::Back, last + unit::ByteLength{1U})});
    case LogNameFormat::LeftTruncated:
        if (settings.nameLimit().isZero() || value.characterLength() <= settings.nameLimit()) {
            return value;
        }
        return value.truncated(settings.nameLimit(), text::TruncateMode::Begin, settings.truncationMark());
    }
    return value;
}

auto LogLineFormatter::messageText(const text::String &message, const LogLineFormat &settings) -> text::String {
    switch (settings.messageTruncation().toRawValue()) {
    case LogMessageTruncation::None:
    case LogMessageTruncation::TotalLineLength:
        return message;
    case LogMessageTruncation::FirstLine: {
        const auto lineBreak = message.find("\n"_el);
        if (lineBreak.isNoIndex()) {
            return message;
        }
        return text::String::fromJoined({
            message.slice(text::StringSide::Front, lineBreak),
            settings.truncationMark(),
        });
    }
    case LogMessageTruncation::CharacterCount:
        if (settings.messageLimit().isZero() || message.characterLength() <= settings.messageLimit()) {
            return message;
        }
        return message.truncated(settings.messageLimit(), text::TruncateMode::End, settings.truncationMark());
    }
    return message;
}

auto LogLineFormatter::truncatedMessageForTotalLength(
    const LogLine &line, const text::String &message, const LogLineFormat &settings) -> text::String {
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
    return message.truncated((maximum - fixedLength) / occurrences, text::TruncateMode::End, settings.truncationMark());
}

auto LogLineFormatter::render(
    const LogLineFormat &settings,
    const text::String &time,
    const text::String &level,
    const text::String &name,
    const text::String &message) -> LogLineConstPtr {
    auto segments = std::vector<LogLineSegment>{};
    const auto appendLiteral = [&segments](text::String literal) -> void {
        if (!literal.isEmpty()) {
            segments.push_back(LogLineSegment{LogLinePart::Literal, std::move(literal)});
        }
    };
    static const auto cBraces = text::CharSet{U'{', U'}'};
    static const auto cCloseBrace = text::CharSet{U'}'};
    auto reader = text::StringCharReader{settings.pattern()};
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
            appendLiteral(text::String::fromJoined({"{"_el, key}));
            break;
        }
        if (const auto part = LogLinePart::fromString(key); part.has_value()) {
            switch (part->toRawValue()) {
            case LogLinePart::Time:
                segments.push_back({*part, time});
                break;
            case LogLinePart::Level:
                segments.push_back({*part, level});
                break;
            case LogLinePart::Name:
                segments.push_back({*part, name});
                break;
            case LogLinePart::Message:
                segments.push_back({*part, message});
                break;
            case LogLinePart::Literal:
            case LogLinePart::_Count:
                break;
            }
        } else {
            segments.push_back({LogLinePart::Literal, text::String::fromJoined({"{"_el, key, "}"_el})});
        }
        reader.startCapture();
    }
    appendLiteral(reader.takeBuffer().toString());
    return std::make_shared<LogLine>(std::move(segments));
}

}
