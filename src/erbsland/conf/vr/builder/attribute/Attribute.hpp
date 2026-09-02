// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../RuleDefinition.hpp"

#include "../../../../text/String.hpp"

namespace erbsland::conf::vr::builder {

/// Base interface for validation-rule builder attributes.
/// Custom attributes may implement this interface without depending on internal rule types.
/// @tested{VrBuilderApiTest}
class Attribute {
public:
    // defaults
    virtual ~Attribute() = default;
    /// Apply this attribute to a rule under construction.
    /// @param rule The rule definition to modify.
    virtual void apply(RuleDefinition &rule) const = 0;
    /// Apply this attribute using function-call syntax.
    /// @param rule The rule definition to modify.
    void operator()(RuleDefinition &rule) const { apply(rule); }

protected:
    /// Throw an error that explains invalid attribute data.
    /// @param message The validation error description.
    [[noreturn]] static void throwValidationError(text::String message);
};

}
