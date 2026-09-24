// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/vr/RulesBuilder.hpp>

namespace demo {

/// Distinguish supplied values, rule defaults, and deliberately unvalidated branches.
///
/// `isDefaultValue()` identifies values inserted by validation. `wasValidated()` distinguishes a parsed value from one
/// that has passed through a rule set; a `NotValidated` rule is still attached as an explicit schema decision.
void defaultsAndValidation() {
    using el::conf::vr::RuleType;
    using el::conf::vr::builder::Default;

    auto builder = el::conf::vr::RulesBuilder{};
    builder.addRule("survey"_el, RuleType::Section);
    builder.addRule("survey.forest"_el, RuleType::Text);
    builder.addRule("survey.region"_el, RuleType::Text, Default{"Mata Atlântica"_el});
    builder.addRule("survey.extension"_el, RuleType::NotValidated);
    const auto rules = builder.takeRules();

    const auto configuration = "[survey]\n"
                               "forest: \"Floresta Nacional de Ipanema\"\n"
                               "extension: \"vendor-owned value\"\n"_el;
    const auto uncheckedDocument = el::conf::Parser{}.parseTextOrThrow(configuration);
    const auto document = el::conf::Parser{}.parseTextOrThrow(configuration);
    rules->validate(document, 1);

    const auto forest = document->valueOrThrow("survey.forest"_el);
    const auto region = document->valueOrThrow("survey.region"_el);
    const auto extension = document->valueOrThrow("survey.extension"_el);
    const auto yesNo = el::BooleanFormat::yesNo();

    el::io::printLine(
        "Forest validated before rules: "_el,
        yesNo,
        uncheckedDocument->valueOrThrow("survey.forest"_el)->wasValidated());
    el::io::printLine("Forest validated after rules: "_el, yesNo, forest->wasValidated());
    el::io::printLine("Region is a default: "_el, yesNo, region->isDefaultValue());
    el::io::printLine("Extension rule: "_el, extension->validationRule()->type().toText());
}

}
