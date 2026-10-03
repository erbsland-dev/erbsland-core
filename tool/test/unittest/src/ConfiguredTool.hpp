// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "NoConfigurationTool.hpp"

#include <erbsland/conf/vr/RulesBuilder.hpp>

/// Provide rules for the probe section independently of other registered tools.
/// @tested{ToolApplicationTest}
class ConfiguredTool final : public NoConfigurationTool {
public: // implement Tool
    [[nodiscard]] auto configurationRules() const -> el::conf::vr::RulesPtr override {
        auto builder = el::conf::vr::RulesBuilder{};
        builder.addRule("message"_el, el::conf::vr::RuleType::Text, el::conf::vr::builder::Default{"default probe"_el});
        return builder.takeRules();
    }
};
