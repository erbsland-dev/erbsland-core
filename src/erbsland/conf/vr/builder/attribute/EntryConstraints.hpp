// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Attribute.hpp"

#include "../constraint/ConstraintAttribute.hpp"

#include <concepts>
#include <memory>
#include <utility>
#include <vector>

namespace erbsland::conf::vr::builder {

/// Configure constraints on the implicit scalar entry of a value list.
/// @seedoc{/reference/conf/validation_rules}
/// @tested{VrEntryTypeTest}
class EntryConstraints final : public Attribute {
public:
    /// Own constraints to apply after EntryType is known, in the supplied order.
    /// @tparam tConstraints Concrete builder constraint types.
    /// @param constraints The constraints to copy or move into this attribute.
    template <typename... tConstraints>
        requires(std::derived_from<tConstraints, ConstraintAttribute> && ...)
    explicit EntryConstraints(tConstraints... constraints) :
        _attributes{std::make_shared<tConstraints>(std::move(constraints))...} {}

public: // implement Attribute
    void apply(RuleDefinition &rule) const override;

private:
    std::vector<std::shared_ptr<const Attribute>> _attributes; ///< Owned constraints in application order.
};

}
