// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/vr/RulesBuilder.hpp>

namespace demo {

/// Apply the secret marker from a validation rule when configuration text leaves the document.
///
/// `isSecret()` communicates handling policy; it does not hide the value or make logging safe by itself. Mark an
/// extracted string as sensitive before retaining it so its shared storage receives the library's guarded cleanup
/// behavior.
void secretValues() {
    using el::conf::vr::RuleType;
    using el::conf::vr::builder::IsSecret;

    auto builder = el::conf::vr::RulesBuilder{};
    builder.addRule("survey"_el, RuleType::Section);
    builder.addRule("survey.api_token"_el, RuleType::Text, IsSecret{});
    const auto rules = builder.takeRules();

    const auto document = el::conf::Parser{}.parseTextOrThrow(
        "[survey]\n"
        "api_token: \"bosque-7a91\"\n"_el);
    rules->validate(document, 1);

    const auto tokenValue = document->valueOrThrow("survey.api_token"_el);
    auto token = tokenValue->asTextOrThrow();
    if (tokenValue->isSecret()) {
        token.markAsSensitive();
    }

    el::io::printLine("Rule marks value as secret: "_el, el::BooleanFormat::yesNo(), tokenValue->isSecret());
    el::io::printLine("Extracted text is sensitive: "_el, el::BooleanFormat::yesNo(), token.isSensitive());
}

}
