// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/vr/RulesBuilder.hpp>

namespace demo {

/// Build validation rules directly in C++.
///
/// `RulesBuilder` creates the same reusable validation model as a rules document. This form is useful when the schema
/// belongs to the executable and should be checked by the C++ compiler along with the code that consumes it.
void compiledRules(const el::Path &configurationPath) {
    using el::conf::vr::RuleType;
    using el::conf::vr::builder::Maximum;
    using el::conf::vr::builder::Minimum;

    auto builder = el::conf::vr::RulesBuilder{};
    builder.addRule("patch"_el, RuleType::Section);
    builder.addRule("patch.name"_el, RuleType::Text);
    builder.addRule("patch.voices"_el, RuleType::Integer, Minimum(1), Maximum(32));
    builder.addRule("patch.oscillator"_el, RuleType::Section);
    builder.addRule("patch.oscillator.waveform"_el, RuleType::Text);
    const auto rules = builder.takeRules();

    const auto document = el::conf::Parser{}.parseFileOrThrow(configurationPath);
    rules->validate(document, 1);

    el::io::printLine("Validated patch: "_el, document->getTextOrThrow("patch.name"_el));
}

}
