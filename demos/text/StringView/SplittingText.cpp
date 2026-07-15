// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// `StringViewList::fromSplit()` splits text into a list of string views.
///
/// The split parts refer to the original text and are therefore cheap to
/// create. Empty parts are dropped by default; set `keepEmpty` to true when an
/// empty field is meaningful. The split limit is the maximum number of split
/// points to apply, so a limit of two produces at most three parts.
void splittingText() {
    const auto table = el::StringView{"dag;plats;väder\n"
                                      "12;Åsleden;klar\n"
                                      "13;;dimma\n"
                                      "14;Nordljus;stjärnklart"_el};
    const auto rows = el::StringViewList::fromSplit(table, el::CharSet{"\n"_elv});

    el::io::printLine("Rows: "_el, rows.count());
    for (const auto &row : rows) {
        const auto fields =
            el::StringViewList::fromSplit(row, el::CharSet{";"_elv}, el::ElementCount::infinite(), true);
        el::io::printLine("  "_el, fields.join(" | "_el));
    }

    // A split limit leaves the unsplit remainder in the last part.
    const auto limited = el::StringViewList::fromSplit(rows.last(), el::CharSet{";"_elv}, el::ElementCount{1U}, true);
    el::io::printLine("Limited split: "_el, limited.join(" / "_el));
}

}
