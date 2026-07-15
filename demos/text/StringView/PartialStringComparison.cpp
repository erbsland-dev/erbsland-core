// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Use `startsWith()`, `endsWith()`, `contains()`, and `count()` to test parts of a string.
/// These methods use the same comparison rules as whole-string comparison and accept the
/// same optional character comparison function. Prefix and suffix tests are efficient
/// even for large strings because only the required edge of the string is inspected.
void partialStringComparison() {
    const auto observationLog = "Lachtan: LEDOVÁ KRA; lachtan: tiché moře; tuleň: ledová kra; mrož: severní útes"_elv;
    const auto booleanFormat = el::BooleanFormat::yesNo();

    el::io::printLine("Observation Log:"_el);
    el::io::printLine(observationLog);
    el::io::printLine();

    // Case-sensitive partial tests are direct and predictable.
    el::io::printLine(
        "startsWith(\"Lachtan\"_el) ................: "_el, booleanFormat, observationLog.startsWith("Lachtan"_el));
    el::io::printLine(
        "endsWith(\"severní útes\"_el) .............: "_el, booleanFormat, observationLog.endsWith("severní útes"_el));
    el::io::printLine(
        "contains(\"tiché moře\"_el) ...............: "_el, booleanFormat, observationLog.contains("tiché moře"_el));
    el::io::printLine("count(\"ledová kra\"_el) ..................: "_el, observationLog.count("ledová kra"_el));

    // Pass a comparison function when matching should ignore case.
    el::io::printLine();
    el::io::printLine(
        "startsWith(\"lachtan\"_el, case-folded) ...: "_el,
        booleanFormat,
        observationLog.startsWith("lachtan"_el, el::Char::compareCaseFolded));
    el::io::printLine(
        "contains(\"LEDOVÁ KRA\"_el, case-folded) ..: "_el,
        booleanFormat,
        observationLog.contains("LEDOVÁ KRA"_el, el::Char::compareCaseFolded));
    el::io::printLine(
        "count(\"lachtan\"_el, case-folded) ........: "_el,
        observationLog.count("lachtan"_el, el::Char::compareCaseFolded));
    el::io::printLine(
        "count(\"ledová kra\"_el, case-folded) .....: "_el,
        observationLog.count("ledová kra"_el, el::Char::compareCaseFolded));
    el::io::printLine("count(\"\"_el) ............................: "_el, observationLog.count(""_el));
}

}
