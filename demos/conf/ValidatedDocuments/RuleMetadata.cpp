// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/vr/RulesBuilder.hpp>

namespace demo {

/// Read presentation metadata from the rule that validated a value.
///
/// Successful validation attaches the matching immutable rule to every validated value. Its title and description can
/// give configuration editors and diagnostic tools useful, application-owned labels without duplicating schema text.
void ruleMetadata() {
    using el::conf::vr::RuleType;
    using el::conf::vr::builder::Description;
    using el::conf::vr::builder::Title;

    auto builder = el::conf::vr::RulesBuilder{};
    builder.addRule("survey"_el, RuleType::Section);
    builder.addRule(
        "survey.forest"_el,
        RuleType::Text,
        Title{"Floresta observada"_el},
        Description{"Nome da área florestal incluída no levantamento."_el});
    const auto rules = builder.takeRules();

    const auto document = el::conf::Parser{}.parseTextOrThrow(
        "[survey]\n"
        "forest: \"Parque Nacional da Tijuca\"\n"_el);
    rules->validate(document, 1);

    // The attached rule is available after validation and can supply display metadata.
    const auto forest = document->valueOrThrow("survey.forest"_el);
    const auto rule = forest->validationRule();
    el::io::printLine("Title: "_el, rule->title());
    el::io::printLine("Description: "_el, rule->description());
}

}
