// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/system/EnvironmentVariables.hpp>
#include <erbsland/text/StringMap.hpp>

namespace demo {

/// Read process environment variables while parsing configuration text.
///
/// The built-in `env` placeholder source is opt-in. Its parameter is the platform environment-variable name, and its
/// result becomes ordinary configuration text. Missing variables produce `undefined`, while tab and newline are
/// preserved and other control or format characters are removed from process environment values.
void builtInEnvironmentSource() {
    auto environment = el::system::EnvironmentVariables{};
    environment.setOrThrow("ERBSLAND_DEMO_RESEARCH_TITLE"_el, "Χαρτογράφηση ωκεάνιων ρευμάτων"_el);
    environment.setOrThrow("ERBSLAND_DEMO_RESEARCH_REGION"_el, "Αιγαίο\t Πέλαγος"_el);
    environment.removeOrThrow("ERBSLAND_DEMO_RESEARCH_MISSING"_el);

    auto parser = el::conf::Parser{};
    parser.addPlaceholderEnvironmentSource();
    const auto document = parser.parseTextOrThrow(
        "[research]\n"
        "title: \"${env:ERBSLAND_DEMO_RESEARCH_TITLE}\"\n"
        "region: \"${env:ERBSLAND_DEMO_RESEARCH_REGION}\"\n"
        "missing: \"${env:ERBSLAND_DEMO_RESEARCH_MISSING}\"\n"_el);

    environment.removeOrThrow("ERBSLAND_DEMO_RESEARCH_TITLE"_el);
    environment.removeOrThrow("ERBSLAND_DEMO_RESEARCH_REGION"_el);

    el::io::printLine("Title: "_el, document->getTextOrThrow("research.title"_el));
    el::io::printLine("Region: "_el, document->getTextOrThrow("research.region"_el));
    el::io::printLine("Missing: "_el, document->getTextOrThrow("research.missing"_el));
}

/// Provide application-owned values through the built-in `var` placeholder source.
///
/// `setPlaceholderVariableSource()` copies a map into the parser and enables the `var` source. Variable names follow
/// regular ELCL name matching, so `collection title` and `COLLECTION_TITLE` select the same entry.
void builtInVariableSource() {
    auto parser = el::conf::Parser{};
    parser.setPlaceholderVariableSource(
        el::StringMap<el::String>{{
            {"collection title"_el, "Σκαθάρια του Ολύμπου"_el},
            {"featured species"_el, "Carabus olympiae"_el},
        }});
    const auto document = parser.parseTextOrThrow(
        "[exhibit]\n"
        "title: \"${var:COLLECTION_TITLE}\"\n"
        "featured species: \"${var:featured species}\"\n"_el);

    el::io::printLine("Exhibit: "_el, document->getTextOrThrow("exhibit.title"_el));
    el::io::printLine("Featured species: "_el, document->getTextOrThrow("exhibit.featured species"_el));
}

}
