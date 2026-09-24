// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "IndividualValuesDemos.hpp"

#include <erbsland/conf/Parser.hpp>

namespace demo {

/// Inspect raw value lists and normalize scalar or list-shaped input.
///
/// `asValueList()` strictly requires a value-list node. `toValueList()` and `toValueMatrix()` are deliberately more
/// relaxed: they also wrap scalar values and turn flat or nested lists into a uniform structural view.
void valueLists() {
    const auto configuration = "[survey]\n"
                               "lead_observer: \"Inês\"\n"
                               "species: \"lobo-guará\", \"jaguatirica\", \"capivara\"\n"
                               "observations:\n"
                               "    * 4, 6, 3\n"
                               "    * 2, 5\n"_el;
    const auto document = el::conf::Parser{}.parseTextOrThrow(configuration);
    const auto speciesValue = document->valueOrThrow("survey.species"_el);

    // Strict list access keeps the individual Value objects and their metadata.
    const auto species = speciesValue->asValueListOrThrow();
    el::io::printLine("First species: "_el, speciesValue->firstValue()->asTextOrThrow());
    el::io::printLine("Last species: "_el, speciesValue->lastValue()->asTextOrThrow());
    el::io::printLine("Species entries: "_el, species.size());

    // Relaxed structural conversion also gives a scalar a one-element list view.
    const auto leadObserver = document->valueOrThrow("survey.lead_observer"_el)->toValueList();
    const auto observationMatrix = document->valueOrThrow("survey.observations"_el)->toValueMatrix();
    el::io::printLine("Lead-observer entries: "_el, leadObserver.size());
    el::io::printLine(
        "Observation matrix: "_el,
        observationMatrix.rowCount(),
        " rows, "_el,
        observationMatrix.columnCount(),
        " columns"_el);
}

}
