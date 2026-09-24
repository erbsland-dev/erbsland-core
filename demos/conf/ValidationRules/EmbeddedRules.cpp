// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ValidationRulesDemos.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/vr/Rules.hpp>
#include <erbsland/resource/Resources.hpp>

namespace demo {

/// Load and apply an ELCL validation-rules document embedded in the executable.
///
/// Compiled resources keep a readable rules file beside the source code without introducing a deployment-time path.
/// Parse that text, create a reusable `Rules` instance, and validate each application document before reading values.
void embeddedRules() {
    // Load the rules document from the application's compiled resources.
    const auto ruleText =
        el::application().resources().getTextOrThrow("conf-validation-rules"_el, "survey-rules.elcl"_el);
    const auto ruleDocument = el::conf::Parser{}.parseTextOrThrow(ruleText);
    const auto rules = el::conf::vr::Rules::createFromDocument(ruleDocument);

    const auto configuration = "[survey]\n"
                               "forest: \"Parque Nacional da Tijuca\"\n"
                               "observers: 8\n"_el;
    const auto document = el::conf::Parser{}.parseTextOrThrow(configuration);

    // Validation checks the shape and constraints, then adds configured default values.
    rules->validate(document, 1);
    el::io::printLine("Forest: "_el, document->getTextOrThrow("survey.forest"_el));
    el::io::printLine("Region: "_el, document->getTextOrThrow("survey.region"_el));
}

}
