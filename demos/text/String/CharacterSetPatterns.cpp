// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/BooleanFormat.hpp>
#include <erbsland/CharSet.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>

namespace demo {

/// `CharSet::fromPattern()` creates compact character sets from literal and range patterns.
///
/// A hyphen between two characters defines a range.
/// A leading or trailing hyphen is treated as a literal hyphen.
/// A leading caret selects every Unicode scalar value outside the following set.
void characterSetPatterns() {
    auto identifierChars = el::CharSet::fromPattern("_a-zA-Z0-9"_el);
    auto optionNameChars = el::CharSet::fromPattern("-_a-zA-Z0-9"_el);

    const auto stationId = el::String{"ORBIT_07"_el};
    const auto optionName = el::String{"orbite-07"_el};
    const auto spacedName = el::String{"orbite 07"_el};

    const auto invalidOptionChars = el::CharSet::fromPattern("^-_a-zA-Z0-9"_el);
    el::io::printLine("Filtered option ...........: "_el, spacedName.removedAll(invalidOptionChars));

    const auto yesNo = el::BooleanFormat::yesNo();
    el::io::printLine("Identifier \"", stationId, "\" ....: "_el, yesNo, stationId.containsOnly(identifierChars));
    el::io::printLine("Option \"", optionName, "\" ........: "_el, yesNo, optionName.containsOnly(optionNameChars));
    el::io::printLine("Option \"", spacedName, "\" ........: "_el, yesNo, spacedName.containsOnly(optionNameChars));
}

}
