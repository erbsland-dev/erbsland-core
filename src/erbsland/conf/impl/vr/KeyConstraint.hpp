// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Constraint.hpp"
#include "ConstraintHandlerContext.hpp"
#include "KeyConstraint_fwd.hpp"

#include <vector>

namespace erbsland::conf::impl {

/// Constraint that requires references to configured key paths.
class KeyConstraint final : public Constraint {
public:
    using KeyReference = NamePath;
    using KeyReferences = std::vector<KeyReference>;

public:
    /// Create a constraint that requires key references.
    /// @param keyReferences The required key references.
    explicit KeyConstraint(KeyReferences keyReferences);

public:
    /// Get the configured key references.
    [[nodiscard]] auto getKeyReferences() const -> const KeyReferences &;

private:
    KeyReferences _keyReferences;
};

/// Create a key constraint from its parsed definition.
auto handleKeyConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr;

}
