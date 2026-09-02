// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringPartConstraint.hpp"

#include <utility>

namespace erbsland::conf::vr::builder {

using namespace text::literals;

/// Adds a contains text constraint.
class Contains final : public StringPartConstraint {
public:
    /// Creates a contains constraint from expected text parts.
    /// @param values The expected text parts.
    /// @param options Additional constraint options.
    explicit Contains(text::StringList values, ConstraintOptions options = {}) :
        StringPartConstraint(Kind::Contains, std::move(values), std::move(options)) {}
    /// Creates a contains constraint from one expected text part.
    /// @param value The expected text part.
    /// @param options Additional constraint options.
    explicit Contains(const text::String &value, ConstraintOptions options = {}) :
        StringPartConstraint(Kind::Contains, value, std::move(options)) {}
    /// Creates a contains constraint from expected text parts.
    /// @param values The expected text parts.
    /// @param options Additional constraint options.
    explicit Contains(const std::initializer_list<text::String> values, ConstraintOptions options = {}) :
        StringPartConstraint(Kind::Contains, values, std::move(options)) {}
};

}
