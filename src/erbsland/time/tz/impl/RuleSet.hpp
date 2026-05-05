// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Rule.hpp"

#include <span>

namespace erbsland::time::tz::impl {

/// A generated contiguous set of DST rules.
/// @notest{Internal generated-data helper.}
struct RuleSet final {
    std::span<const Rule> rules;
};

}
