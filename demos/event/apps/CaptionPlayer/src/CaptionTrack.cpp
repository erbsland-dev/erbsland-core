// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CaptionTrack.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/PathContent.hpp>
#include <erbsland/re/Match.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/U8StringConstIterator.hpp>

namespace demo {

using namespace el::text::literals;

auto CaptionTrack::parseTime(const el::String &text) -> std::optional<el::TimeDelta> {
    static const auto cTime = el::RegEx::compile(R"(([0-9]{2}):([0-5][0-9]):([0-5][0-9]),([0-9]{3}))"_el);
    const auto match = cTime->fullMatch(text);
    if (!match) {
        return {};
    }
    const auto hours = match->content(1).toIntegerOrThrow<int>();
    const auto minutes = match->content(2).toIntegerOrThrow<int>();
    const auto seconds = match->content(3).toIntegerOrThrow<int>();
    const auto millis = match->content(4).toIntegerOrThrow<int>();
    return el::TimeDelta::milliseconds(((hours * 60 + minutes) * 60 + seconds) * 1000 + millis);
}

auto CaptionTrack::read(const el::Path &path) -> CaptionTrack {
    const auto lines = el::StringList::fromSplit(
        path.content().readTextOrThrow(), el::CharSet{U'\n'}, el::ItemCount::infinite(), true);
    const auto end = lines.count();
    auto line = el::ItemIndex{};
    auto cues = std::vector<CaptionCue>{};
    const auto error = [&](const el::String &reason) {
        return el::ApplicationError{el::StringFormat{"{} at line {} (cue {}): {}"_el}.build(
            path.toNativeString(), line.toSizeT() + 1, cues.size() + 1, reason)};
    };
    const auto lineText = [&]() { return lines.get(line).trimmed(el::CharSet{U'\r'}, el::StringSide::Back); };
    static const auto cNumber = el::RegEx::compile("[0-9]+"_el);
    static const auto cSpan = el::RegEx::compile(R"(([^ \t]+)[ \t]+-->[ \t]+([^ \t]+))"_el);
    while (line.isWithin(end)) {
        if (lineText().trimmed().isEmpty()) {
            ++line;
            continue;
        }
        auto cue = CaptionCue{};
        cue.number = lineText().trimmed();
        if (!cNumber->fullMatch(cue.number)) {
            throw error("Expected a numeric SubRip cue label."_el);
        }
        ++line;
        if (!line.isWithin(end)) {
            throw error("Missing cue timing line."_el);
        }
        const auto span = cSpan->fullMatch(lineText().trimmed());
        if (!span) {
            throw error("Expected start --> end."_el);
        }
        const auto start = parseTime(span->content(1));
        const auto finish = parseTime(span->content(2));
        if (!start || !finish || *start >= *finish) {
            throw error("Expected HH:MM:SS,mmm timestamps with end after start."_el);
        }
        cue.start = *start;
        cue.end = *finish;
        ++line;
        while (line.isWithin(end) && !lineText().trimmed().isEmpty()) {
            const auto text = lineText();
            for (const auto character : text) {
                if (character.isControl() && character != U'\t') {
                    throw error("Unexpected control character in subtitle text."_el);
                }
            }
            cue.lines.append(text);
            ++line;
        }
        if (cue.lines.isEmpty()) {
            throw error("Expected subtitle text."_el);
        }
        cues.push_back(std::move(cue));
    }
    if (cues.empty()) {
        throw error("The subtitle track is empty."_el);
    }
    return CaptionTrack{path.name(), std::move(cues)};
}

}
