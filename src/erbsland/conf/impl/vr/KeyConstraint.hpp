// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Constraint.hpp"
#include "ConstraintHandlerContext.hpp"

#include <vector>

namespace erbsland::conf::impl {

class KeyConstraint final : public Constraint {
public:
    using KeyReference = NamePath;
    using KeyReferences = std::vector<KeyReference>;

public:
    explicit KeyConstraint(KeyReferences keyReferences);

public:
    [[nodiscard]] auto getKeyReferences() const -> const KeyReferences &;

private:
    KeyReferences _keyReferences;
};

auto handleKeyConstraint(const ConstraintHandlerContext &context) -> ConstraintPtr;

}
