// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "IndividualValuesDemos.hpp"

#include <erbsland/conf/Parser.hpp>

namespace demo {

/// Read type-checked lists and matrices from a document.
///
/// Typed collection getters resolve the path, verify every element, and return native values. They also accept a scalar
/// of the requested type as a one-element list or matrix, which lets a setting support concise and expanded forms.
void typedCollections() {
    const auto configuration = "[survey]\n"
                               "species: \"onça-pintada\", \"tamanduá-bandeira\"\n"
                               "sample_count: 12\n"
                               "observations:\n"
                               "    * 4, 6, 3\n"
                               "    * 2, 5, 1\n"_el;
    const auto document = el::conf::Parser{}.parseTextOrThrow(configuration);

    // Typed getters return native collections after checking every element.
    const auto species = document->getListOrThrow<el::String>("survey.species"_el);
    const auto sampleCounts = document->getListOrThrow<int>("survey.sample_count"_el);
    const auto observations = document->getMatrixOrThrow<int>("survey.observations"_el);

    el::io::printLine("Species: "_el, species.size());
    el::io::printLine("Sample-count entries: "_el, sampleCounts.size());
    el::io::printLine("First observation: "_el, observations.valueOrThrow({}, {}));
}

}
