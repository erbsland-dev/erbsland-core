// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Attribute.hpp"

namespace erbsland::conf::vr::builder {

/// Select the type of an implicit scalar `vr_entry` rule for a value list.
/// @seedoc{/reference/conf/validation_rules}
/// @tested{VrEntryTypeTest}
class EntryType final : public Attribute {
public:
    /// Select the scalar type accepted by each list entry.
    /// @param type A scalar rule type, including `RuleType::Value` for any scalar.
    explicit EntryType(RuleType type) : _type{type} {}

public: // implement Attribute
    void apply(RuleDefinition &rule) const override;

private:
    RuleType _type; ///< The implicit entry's scalar type.
};

}
