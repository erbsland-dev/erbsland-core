// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// StringView::find can be called repeatedly to collect every text position.
///
/// Pass a start position to continue the search after a previous match. Move
/// the start position with advance when overlapping matches should remain
/// possible, or add the needle length when only non-overlapping matches are
/// useful. A character comparison function can adapt the matching rule without
/// first transforming the source text.
void findingAllOccurrences() {
    const auto missionLog = el::StringView{"ROV Freja såg ljus; rov freja markerade ljus; ROV Freja sparade karta"_el};
    const auto needle = el::StringView{"rov freja"_el};

    el::io::printLine("Mission log: "_el, missionLog);
    el::io::printLine("Needle: "_el, needle);
    el::io::printLine("Case-folded matches:"_el);

    auto searchStart = el::ByteIndex::zero();
    auto matchIndex = missionLog.find(needle, searchStart, el::Char::compareCaseFolded);
    while (!matchIndex.isNoIndex()) {
        const auto match = missionLog.slice(el::ByteRange{matchIndex, needle.length()});
        el::io::printLine("  byte "_el, matchIndex, ": "_el, match);

        searchStart = matchIndex;
        if (!missionLog.advance(searchStart)) {
            break;
        }
        matchIndex = missionLog.find(needle, searchStart, el::Char::compareCaseFolded);
    }
}

}
