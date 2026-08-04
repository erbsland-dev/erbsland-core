// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Attribute.hpp"

#include "../../../vr/RuleType.hpp"

namespace erbsland::conf::vr::builder {

/// Sets the rule type.
class Type : public Attribute {
public:
    /// Set the rule value type.
    /// @param type The value type to set.
    explicit Type(const RuleType type) : _type{type} {}
    void operator()(Rule &rule) override;
    RuleType _type;
};

}
