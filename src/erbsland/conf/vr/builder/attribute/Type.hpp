// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Attribute.hpp"

#include "../../../vr/RuleType.hpp"

namespace erbsland::conf::vr::builder {

/// Sets the rule type.
struct Type : Attribute {
    explicit Type(const RuleType type) : _type{type} {}
    void operator()(impl::Rule &rule) override;
    RuleType _type;
};

}
