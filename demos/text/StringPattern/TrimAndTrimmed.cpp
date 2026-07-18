// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// `trimmed()` and `trim()` remove the matched front or back shell of a string pattern.
///
/// For a pattern with text on both sides of the divider, the divider represents the part that stays. This is useful
/// when a protocol-like string wraps the meaningful value in a predictable prefix and suffix.
void trimAndTrimmed() {
    const auto envelope = el::StringPattern{"messung:*;ok"_el};
    const auto original = el::String{"messung:temperatur=21.4C;ok"_el};

    el::io::printLine("original ................: "_el, original);
    el::io::printLine("trimmed .................: "_el, envelope.trimmed(original));

    auto editable = el::StringEditor{"messung:ph=7.1;ok"_el};
    if (envelope.trim(editable)) {
        el::io::printLine("mutable .................: "_el, editable);
    }

    auto unchanged = el::StringEditor{"messung:ph=7.1;prüfen"_el};
    el::io::printLine("changed .................: "_el, envelope.trim(unchanged));
    el::io::printLine("kept ....................: "_el, unchanged);
}

}
