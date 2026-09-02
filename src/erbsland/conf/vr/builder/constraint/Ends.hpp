// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringPartConstraint.hpp"

#include <utility>

namespace erbsland::conf::vr::builder {

using namespace text::literals;

/// Adds an ends-with text constraint.
class Ends final : public StringPartConstraint {
public:
    /// Creates an ends-with constraint from expected suffixes.
    /// @param values The expected suffixes.
    /// @param options Additional constraint options.
    explicit Ends(text::StringList values, ConstraintOptions options = {}) :
        StringPartConstraint(Kind::Ends, std::move(values), std::move(options)) {}
    /// Creates an ends-with constraint from one expected suffix.
    /// @param value The expected suffix.
    /// @param options Additional constraint options.
    explicit Ends(const text::String &value, ConstraintOptions options = {}) :
        StringPartConstraint(Kind::Ends, value, std::move(options)) {}
    /// Creates an ends-with constraint from expected suffixes.
    /// @param values The expected suffixes.
    /// @param options Additional constraint options.
    explicit Ends(const std::initializer_list<text::String> values, ConstraintOptions options = {}) :
        StringPartConstraint(Kind::Ends, values, std::move(options)) {}
};

}
