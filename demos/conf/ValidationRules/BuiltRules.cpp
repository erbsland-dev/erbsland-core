// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ValidationRulesDemos.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/vr/RulesBuilder.hpp>

namespace demo {

/// Build and apply validation rules directly in C++.
///
/// `RulesBuilder` combines each rule type with attributes and constraints. Attributes describe behavior or metadata;
/// constraints narrow the accepted value. Finalization checks the definition and returns an immutable, reusable rule
/// set.
void builtRules() {
    using el::conf::vr::RuleType;
    using el::conf::vr::builder::Default;
    using el::conf::vr::builder::Description;
    using el::conf::vr::builder::IsOptional;
    using el::conf::vr::builder::IsSecret;
    using el::conf::vr::builder::Maximum;
    using el::conf::vr::builder::Minimum;
    using el::conf::vr::builder::Title;

    auto builder = el::conf::vr::RulesBuilder{};
    builder.configureRoot(Title{"Levantamento florestal"_el});
    builder.addRule("survey"_el, RuleType::Section, Title{"Levantamento da floresta"_el});
    builder.addRule(
        "survey.forest"_el, RuleType::Text, Description{"Nome da floresta observada"_el}, Minimum{3}, Maximum{80});
    builder.addRule("survey.observers"_el, RuleType::Integer, Minimum{1}, Maximum{12});
    builder.addRule("survey.region"_el, RuleType::Text, Default{"Mata Atlântica"_el});
    builder.addRule("survey.api_token"_el, RuleType::Text, IsOptional{}, IsSecret{});
    const auto rules = builder.takeRules();

    const auto configuration = "[survey]\n"
                               "forest: \"Floresta Nacional de Ipanema\"\n"
                               "observers: 6\n"_el;
    const auto document = el::conf::Parser{}.parseTextOrThrow(configuration);
    rules->validate(document, 1);

    el::io::printLine("Validated observers: "_el, document->getIntegerOrThrow("survey.observers"_el));
    el::io::printLine("Default region: "_el, document->getTextOrThrow("survey.region"_el));
}

}
