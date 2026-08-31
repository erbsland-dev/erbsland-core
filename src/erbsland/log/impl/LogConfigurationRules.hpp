// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../conf/vr/Rules_fwd.hpp"

namespace erbsland::log::impl {

/// Build and finalize the logging configuration rules.
/// @return A complete compiled ruleset with user-facing metadata.
/// @tested{LogConfigurationParserTest}
[[nodiscard]] auto createLogConfigurationRules() -> conf::vr::RulesPtr;

}
