// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/vr/Rules.hpp>

namespace demo {

/// Validate configuration with rules written as an ELCL document.
///
/// Parse the rules like any other configuration, compile them with `Rules::createFromDocument()`, and apply the
/// resulting reusable rule set to a parsed document. Validation checks types and constraints before values are used.
void documentRules(const el::Path &configurationPath) {
    const auto ruleText = "[patch]\n"
                          "type: \"section\"\n"
                          "[patch.name]\n"
                          "type: \"text\"\n"
                          "[patch.voices]\n"
                          "type: \"integer\"\n"
                          "minimum: 1\n"
                          "maximum: 32\n"
                          "[patch.oscillator]\n"
                          "type: \"section\"\n"
                          "[patch.oscillator.waveform]\n"
                          "type: \"text\"\n"_el;

    // Compile the rules once, then use them for every document with this schema.
    const auto ruleDocument = el::conf::Parser{}.parseTextOrThrow(ruleText);
    const auto rules = el::conf::vr::Rules::createFromDocument(ruleDocument);
    const auto document = el::conf::Parser{}.parseFileOrThrow(configurationPath);
    rules->validate(document, 1);

    el::io::printLine("Validated voices: "_el, document->getIntegerOrThrow("patch.voices"_el));
}

}
