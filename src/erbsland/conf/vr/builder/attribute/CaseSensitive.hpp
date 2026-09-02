// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Attribute.hpp"

#include "../../../../text/CaseSensitivity.hpp"

namespace erbsland::conf::vr::builder {

/// Sets the case sensitivity used by the rule.
class CaseSensitive : public Attribute {
public:
    /// Set the case sensitivity for a rule.
    /// @param caseSensitivity The requested case sensitivity.
    explicit CaseSensitive(const text::CaseSensitivity caseSensitivity = text::CaseSensitivity::CaseSensitive) :
        _caseSensitivity{caseSensitivity} {}
    void apply(RuleDefinition &rule) const override;
    text::CaseSensitivity _caseSensitivity{text::CaseSensitivity::CaseSensitive};
};

}
