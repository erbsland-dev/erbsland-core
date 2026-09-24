// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "IndividualValuesDemos.hpp"

#include <erbsland/conf/Parser.hpp>

namespace demo {

/// Inspect a value before accepting one of several representations.
///
/// The `is...()` methods make intentional alternatives explicit. Use them when an application genuinely accepts more
/// than one ELCL type; prefer a typed getter when the configuration contract requires exactly one type.
void typeTests() {
    const auto configuration = "[survey]\n"
                               "station: 42\n"
                               "reference_species: \"bugio-ruivo\"\n"_el;
    const auto document = el::conf::Parser{}.parseTextOrThrow(configuration);

    const auto station = document->valueOrThrow("survey.station"_el);
    if (station->isInteger()) {
        el::io::printLine("Numeric station: "_el, station->asIntegerOrThrow());
    } else if (station->isText()) {
        el::io::printLine("Named station: "_el, station->asTextOrThrow());
    }

    const auto referenceSpecies = document->valueOrThrow("survey.reference_species"_el);
    el::io::printLine("Reference is text: "_el, el::BooleanFormat::yesNo(), referenceSpecies->isText());
    el::io::printLine("Document is a map: "_el, el::BooleanFormat::yesNo(), document->isMap());
}

}
