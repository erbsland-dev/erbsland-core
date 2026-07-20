// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Rule.hpp"

namespace erbsland::conf::impl {

/// The context passed to each constraint handler in `RulesFromDocument`.
struct ConstraintHandlerContext {
    RulePtr rule;        ///< The rule to process.
    conf::ValuePtr node; ///< The node with the constraint to be processed.
    bool isNegated;      ///< If the constraint is negated with `not_...`.
};

}
