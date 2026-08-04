// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringPartConstraint.hpp"

#include "../../../impl/vr/ContainsConstraint.hpp"

#include <utility>

namespace erbsland::conf::vr::builder {

using namespace text::literals;

/// Adds a contains text constraint.
class Contains final : public StringPartConstraint<impl::ContainsConstraint> {
public:
    /// Creates a contains constraint from expected text parts.
    /// @param values The expected text parts.
    /// @param options Additional constraint options.
    explicit Contains(text::StringList values, ConstraintOptions options = {}) :
        StringPartConstraint(std::move(values), std::move(options)) {
        _name = "contains"_el;
    }
    /// Creates a contains constraint from one expected text part.
    /// @param value The expected text part.
    /// @param options Additional constraint options.
    explicit Contains(const text::String &value, ConstraintOptions options = {}) :
        StringPartConstraint(value, std::move(options)) {
        _name = "contains"_el;
    }
    /// Creates a contains constraint from expected text parts.
    /// @param values The expected text parts.
    /// @param options Additional constraint options.
    explicit Contains(const std::initializer_list<text::String> values, ConstraintOptions options = {}) :
        StringPartConstraint(values, std::move(options)) {
        _name = "contains"_el;
    }
};

}
