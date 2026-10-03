// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../conf/vr/Rules_fwd.hpp"
#include "../../conf/vr/RulesBuilder.hpp"

namespace erbsland::log::impl {

/// Build and finalize the logging configuration rules.
/// @return A complete compiled ruleset with user-facing metadata.
/// @tested{LogConfigurationParserTest}
[[nodiscard]] auto createLogConfigurationRules() -> conf::vr::RulesPtr;

/// Configure the current root or existing branch as a line-format section and add its relative child rules.
/// @param builder The schema builder rooted at the section to configure.
/// @tested{LogConfigurationParserTest}
void addLogLineFormatRules(conf::vr::RulesBuilder &builder);

}
