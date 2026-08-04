// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Rule_fwd.hpp"
#include "ValidationContext_fwd.hpp"
#include "ValidationTarget.hpp"

#include "../../Value.hpp"

namespace erbsland::conf::impl {

/// The context to validate a constraint.
class ValidationContext {
public:
    /// The target of the constraint validation.
    ValidationTarget target{ValidationTarget::Value};
    /// The value that is being validated.
    conf::ValuePtr value;
    /// The validation rule.
    RulePtr rule;

public:
};

}
