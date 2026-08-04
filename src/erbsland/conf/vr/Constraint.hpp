// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Constraint_fwd.hpp"
#include "ConstraintType.hpp"

#include "../Location.hpp"

namespace erbsland::conf::vr {

/// A constraint for a validation-rule.
class Constraint {
public:
    // defaults
    virtual ~Constraint() = default;

public:
    /// The name.
    [[nodiscard]] virtual auto name() const -> text::String = 0;
    /// The type.
    [[nodiscard]] virtual auto type() const -> ConstraintType = 0;
    /// Test if a custom error message set.
    [[nodiscard]] virtual auto hasCustomError() const -> bool = 0;
    /// The custom error message.
    [[nodiscard]] virtual auto customError() const -> text::String = 0;
    /// If the result of this constraint test is negated.
    [[nodiscard]] virtual auto isNegated() const -> bool = 0;

public: // location
    /// Test if there is location info.
    [[nodiscard]] virtual auto hasLocation() const noexcept -> bool = 0;
    /// Get the location info.
    [[nodiscard]] virtual auto location() const noexcept -> const Location & = 0;
    /// Set the location info.
    virtual void setLocation(const Location &newLocation) noexcept = 0;
};

}
