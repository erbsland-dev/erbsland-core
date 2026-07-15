// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// `StringView::transformed()` can create a canonical text form with a single character-mapping function.
///
/// ASCII-only mappings are useful for machine-readable identifiers because they leave non-ASCII characters untouched
/// and avoid the Unicode database.
void canonicalStationName() {
    const auto displayName = el::StringView{"Module ORBITE-Äther 07"_el};
    auto canonicalName = displayName.transformed(el::Char::toAsciiLowercase);

    el::io::printLine("Display name ..: "_el, displayName);
    el::io::printLine("Canonical .....: "_el, canonicalName);
}

}
