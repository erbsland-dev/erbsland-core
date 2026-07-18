// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// `removeFirst()` changes only the first matching text occurrence.
/// `removeAll()` removes every matching text occurrence or every character from
/// a character set.
///
/// Text matching is decoded Unicode text matching. You can pass a comparison
/// function such as `Char::compareCaseFolded` when case-insensitive matching is
/// required.
void removeFirstAndAll() {
    const auto source = el::String{"mist :: frost :: mist :: aurora"_el};

    auto firstOnly = el::StringEditor{source};
    firstOnly.removeFirst("mist"_el);

    auto allMist = el::StringEditor{source};
    allMist.removeAll("mist"_el);

    auto withoutSeparators = el::StringEditor{source};
    withoutSeparators.removeAll(el::CharSet{": "_el});

    auto folded = el::StringEditor{"Ähre | äHRE | aster"_el};
    folded.removeAll("ähre"_el, el::Char::compareCaseFolded);

    el::io::printLine("Source: "_el, source);
    el::io::printLine("First text removed: "_el, firstOnly);
    el::io::printLine("All text removed: "_el, allMist);
    el::io::printLine("Characters removed: "_el, withoutSeparators);
    el::io::printLine("Case-folded removal: "_el, folded);
}

}
