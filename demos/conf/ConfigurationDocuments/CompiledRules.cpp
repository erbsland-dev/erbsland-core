// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/all_conf_vr.hpp>
#include <erbsland/all_conf_vr_builder.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/StandardStreams.hpp>

namespace demo {

using namespace el::text::literals;

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
    auto patch = builder.createBranch("patch"_el);
    patch.addRule("name"_el, RuleType::Text);
    patch.addRule("voices"_el, RuleType::Integer, Minimum(1), Maximum(32));
    patch.addRule("oscillator"_el, RuleType::Section);
    patch.addRule("oscillator.waveform"_el, RuleType::Text);
    const auto rules = builder.takeRules();

    const auto document = el::conf::Parser{}.parseFileOrThrow(configurationPath);
    rules->validate(document, 1);

    el::io::printLine("Validated patch: "_el, document->getTextOrThrow("patch.name"_el));
}

}
