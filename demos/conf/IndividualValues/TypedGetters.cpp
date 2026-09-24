// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "IndividualValuesDemos.hpp"

#include <erbsland/conf/Parser.hpp>

#include <cstddef>
#include <cstdint>

namespace demo {

/// Read application settings with typed convenience getters.
///
/// Named getters produce compact, expressive code for ordinary settings. Template getters support the same path-like
/// inputs and are useful in generic code or when a native integer width must be selected explicitly.
void typedGetters() {
    const auto configuration = "[survey]\n"
                               "forest: \"Floresta da Tijuca\"\n"
                               "observers: 7\n"
                               "species: \"mico-leão-dourado\", \"tucano\"\n"_el;
    const auto document = el::conf::Parser{}.parseTextOrThrow(configuration);

    // A string path is concise, while a Name or NamePath is convenient when paths are assembled or reused.
    const auto forest = document->getTextOrThrow("survey.forest"_el);
    const auto survey = document->valueOrThrow(el::conf::Name::createRegular("survey"_el));
    const auto observersPath = el::conf::NamePath::fromText("observers"_el);
    const auto observers = survey->getOrThrow<std::uint16_t>(observersPath);

    // An index is the fourth path-like form and addresses an entry in a list directly.
    const auto species = document->valueOrThrow("survey.species"_el);
    const auto firstSpecies = species->getOrThrow<el::String>(std::size_t{0});

    // Non-throwing getters make an application default explicit for optional values.
    const auto season = document->getText("survey.season"_el, "seca"_el);
    el::io::printLine(forest, ": "_el, observers, " observers"_el);
    el::io::printLine("First species: "_el, firstSpecies, "; season: "_el, season);
}

}
