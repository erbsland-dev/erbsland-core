// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/Char.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringTransformations.hpp>

namespace demo {

/// `toAsciiLowercase()` creates a canonical text form while sharing unchanged string storage.
///
/// ASCII-only mappings are useful for machine-readable identifiers because they leave non-ASCII characters untouched
/// and avoid the Unicode database.
void canonicalStationName() {
    const auto displayName = el::String{"Module ORBITE-Äther 07"_el};
    auto canonicalName = el::toAsciiLowercase(displayName);

    el::io::printLine("Display name ..: "_el, displayName);
    el::io::printLine("Canonical .....: "_el, canonicalName);
}

}
