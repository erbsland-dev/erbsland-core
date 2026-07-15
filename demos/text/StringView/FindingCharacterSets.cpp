// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Character-set search functions find positions of decoded characters.
///
/// Use findFirstOf and findLastOf to locate delimiters from a set. Use
/// findFirstNotOf and findLastNotOf to skip padding or other characters that
/// are not part of the useful text. All positions returned by StringView are
/// byte indexes that can be reused for slicing or further searches.
void findingCharacterSets() {
    static const auto padding = el::CharSet::from(el::AsciiCategory::Whitespace);
    static const auto separators = el::CharSet{U':', U'=', U';'};

    const auto logLine = el::StringView{"  Dyklogg: mål=Kosterhavet; djup=240m; status=redo  "_el};

    const auto contentStart = logLine.findFirstNotOf(padding);
    const auto contentEnd = logLine.findLastNotOf(padding);
    const auto firstSeparator = logLine.findFirstOf(separators);
    const auto lastSeparator = logLine.findLastOf(separators);
    const auto statusStart = logLine.find("status"_el);
    const auto separatorBeforeStatus = logLine.findLastOf(separators, statusStart);

    el::io::printLine("Log line: '"_el, logLine, "'"_el);
    el::io::printLine("First content byte ........: "_el, contentStart);
    el::io::printLine("Last content byte .........: "_el, contentEnd);
    el::io::printLine(
        "First separator ...........: "_el, firstSeparator, " ('"_el, logLine.charAt(firstSeparator), "')"_el);
    el::io::printLine(
        "Last separator ............: "_el, lastSeparator, " ('"_el, logLine.charAt(lastSeparator), "')"_el);
    el::io::printLine(
        "Separator before status ...: "_el,
        separatorBeforeStatus,
        " ('"_el,
        logLine.charAt(separatorBeforeStatus),
        "')"_el);
}

}
