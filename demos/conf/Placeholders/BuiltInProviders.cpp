// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PlaceholdersDemos.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/system/EnvironmentVariables.hpp>
#include <erbsland/text/StringMap.hpp>

namespace demo {

/// Enable built-in sources and filters for a marine survey configuration.
void builtInProviders() {
    auto environment = el::system::EnvironmentVariables{};
    environment.setOrThrow("ERBSLAND_DEMO_MARINE_AREA"_el, u8"  瀬戸内海  "_el);

    auto parser = el::conf::Parser{};
    parser.addPlaceholderEnvironmentSource();
    parser.setPlaceholderVariableSource({{{"species"_el, u8"  アマモ  "_el}}});
    parser.addPlaceholderTextFilters();
    const auto document = parser.parseTextOrThrow(
        "[survey]\n"
        "area: \"${env:ERBSLAND_DEMO_MARINE_AREA,required|trim}\"\n"
        "species: \"${var:species|trim}\"\n"_el);

    environment.removeOrThrow("ERBSLAND_DEMO_MARINE_AREA"_el);
    el::io::printLine("Area: "_el, document->getTextOrThrow("survey.area"_el));
    el::io::printLine("Species: "_el, document->getTextOrThrow("survey.species"_el));
}

}
