// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../../text/String.hpp"

#include <memory>

namespace erbsland::conf::impl {
class Constraint;
using ConstraintPtr = std::shared_ptr<Constraint>;
class Rule;
}

namespace erbsland::conf::vr::builder {

/// Optional behavior shared by all builder constraints.
struct ConstraintOptions {
    bool isNegated{false};
    text::String errorMessage{};

    [[nodiscard]] auto prefixedConstraintName(const text::String &constraintName) const -> text::String;

    void applyTo(impl::Constraint &constraint, const text::String &constraintName) const;

    void addToRule(impl::Rule &rule, const impl::ConstraintPtr &constraint, const text::String &constraintName) const;

private:
    [[nodiscard]] static auto withNotPrefix(const text::String &name, const bool isNegated) -> text::String;
};

}
