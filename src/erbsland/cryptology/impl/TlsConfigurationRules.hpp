// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../conf/vr/Rules_fwd.hpp"

namespace erbsland::cryptology::impl {

/// Build and finalize the TLS configuration-entry rules.
/// @return A complete compiled ruleset with user-facing metadata.
/// @tested{TlsConfigurationParserTest}
[[nodiscard]] auto createTlsConfigurationRules() -> conf::vr::RulesPtr;

}
