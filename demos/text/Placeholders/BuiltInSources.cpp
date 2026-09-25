// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PlaceholderDemos.hpp"

#include <erbsland/system/EnvironmentVariables.hpp>
#include <erbsland/text/placeholder/Replacer.hpp>

namespace demo {

/// Read a process value when replacing a field note.
void readEnvironmentSource() {
    auto environment = el::system::EnvironmentVariables{};
    environment.setOrThrow("ERBSLAND_DEMO_VOLCANO_REGION"_el, u8"  Vulkanområdet\tAskja  "_el);

    auto replacer = el::placeholder::Replacer{};
    replacer.addEnvironmentSource();
    replacer.addTextFilters();
    el::io::printLine(replacer.replaceOrThrow("Region: ${env:ERBSLAND_DEMO_VOLCANO_REGION|required|trim}"_el));

    environment.removeOrThrow("ERBSLAND_DEMO_VOLCANO_REGION"_el);
    el::io::printLine(replacer.replaceOrThrow("Unknown: ${env:ERBSLAND_DEMO_VOLCANO_REGION}"_el));
}

/// Supply application values and replace the complete map for a later note.
void readVariableSource() {
    auto replacer = el::placeholder::Replacer{};
    replacer.setVariableSource({{{"field site"_el, u8"Askja"_el}, {"rock type"_el, u8"Basalt"_el}}});
    el::io::printLine(replacer.replaceOrThrow("Site: ${var:FIELD_SITE}; rock: ${var:rock type}"_el));

    replacer.setVariableSource({{{"field site"_el, u8"Hekla"_el}}});
    el::io::printLine(replacer.replaceOrThrow("Next site: ${var:field site}"_el));
}

}
