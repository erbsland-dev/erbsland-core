// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// `StringPattern` parses a compact shell-like pattern once and reuses it for many text checks.
///
/// The parsed syntax is useful for small library-level recognizers: a literal prefix, one decoded character with `?`,
/// bracket character sets, and one `*` divider for front/back matching. Backslash escapes keep special characters
/// literal when they are part of the text format itself.
void patternConstruction() {
    // A pattern can describe the small fixed shell around a laboratory record.
    const auto probeRecord = el::StringPattern{"probe-??[0-9]*"_el};

    const auto probeLines = el::StringList{{
        "probe-AB7 temperatur=21.4C"_el,
        "probe-A7 temperatur=21.4C"_el,
    }};
    const auto lineFormat = el::StringFormat{"{:30}: {}"};
    for (const auto &line : probeLines) {
        el::io::printLine(lineFormat.build(line, probeRecord.matches(line)));
    }

    // Escapes make the pattern syntax available for literal diagnostic markers.
    const auto marker = el::StringPattern{"messung\\[\\?\\]\\*"_el};
    el::io::printLine("escaped marker                : "_el, marker.matches("messung[?]* archiviert"_el));
}

}
