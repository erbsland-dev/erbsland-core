// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

constexpr auto decMax3Digits =
    el::IntegerParseOptions::parserDefault().setFixedBase(el::IntegerBase::Decimal).setMaximumDigits(el::CpLength{3U});

auto readLessonName(el::StringCharReader &reader) -> el::String;
auto readOptionalMinutes(el::StringCharReader &reader, int &minutes) -> bool;
void printPlan(const el::StringView &label, el::StringCharReader reader);

/// `StringCharReader` is the preferred tool for character-by-character parsers.
///
/// - It decodes UTF-8, UTF-16, and UTF-32 through the same API.
/// - It keeps an efficient sequential cursor.
/// - It can save or restore its state when a parser tries an optional grammar branch.
/// - It can capture strings as efficient views to slices of the original text.
///
/// This demo parses a small Turkish study plan.
/// Helper functions receive `StringCharReader` by reference when they consume input.
void studyPlanParser() {
    const auto utf8Plan = el::StringView{"matematik:45;fen:30;müzik"_el};
    printPlan("UTF-8 plan"_el, el::StringCharReader{utf8Plan});

    const auto utf16Plan = el::U16StringView{u"geometri:25;şiir:15"_el};
    printPlan("UTF-16 plan"_el, el::StringCharReader{utf16Plan});

    const auto utf32Plan = el::U32StringView{U"astronomi:40;çizim:20"_el};
    printPlan("UTF-32 plan"_el, el::StringCharReader{utf32Plan});

    const auto draftWithError = el::StringView{"tarih:25;kimya:x"_el};
    printPlan("plan with diagnostic"_el, el::StringCharReader{draftWithError});
}

void printPlan(const el::StringView &label, el::StringCharReader reader) {
    el::io::printLine(label, ":"_el);
    while (!reader.isAtEnd()) {
        const auto lessonStart = reader.position();
        const auto lessonName = readLessonName(reader);
        if (lessonName.isEmpty()) {
            el::io::printLine("  error at index "_el, lessonStart, ": expected lesson name"_el);
            return;
        }

        int minutes = 0;
        if (readOptionalMinutes(reader, minutes)) {
            el::io::printLine("  - "_el, lessonName, ": ", minutes, " min"_el);
        } else {
            el::io::printLine("  - "_el, lessonName, ": no duration"_el);
        }

        if (reader.readIf(U';')) {
            continue;
        }
        if (!reader.isAtEnd()) {
            el::io::printLine(
                "  error at code point "_el, reader.position(), ": unexpected '"_el, reader.peek(), "'"_el);
            return;
        }
    }
}

auto readLessonName(el::StringCharReader &reader) -> el::String {
    const static auto separatorChars = el::CharSet{U':', U';'};
    reader.startCapture();
    reader.readUntil({}, separatorChars);
    return reader.takeCapture().toU8String();
}

auto readOptionalMinutes(el::StringCharReader &reader, int &minutes) -> bool {
    if (!reader.readIf(U':')) {
        return false;
    }
    try {
        minutes = reader.readIntegerOrThrow<int>(decMax3Digits);
        return true;
    } catch (const el::Exception &) {
        return false;
    }
}

}
