// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringPartConstraint.hpp"

#include "../../../impl/vr/StartsConstraint.hpp"

#include <utility>

namespace erbsland::conf::vr::builder {

using namespace text::literals;

/// Adds a starts-with text constraint.
class Starts final : public StringPartConstraint<impl::StartsConstraint> {
public:
    /// Creates a starts-with constraint from expected prefixes.
    /// @param values The expected prefixes.
    /// @param options Additional constraint options.
    explicit Starts(text::StringList values, ConstraintOptions options = {}) :
        StringPartConstraint<impl::StartsConstraint>(std::move(values), std::move(options)) {
        _name = "starts"_el;
    }
    /// Creates a starts-with constraint from one expected prefix.
    /// @param value The expected prefix.
    /// @param options Additional constraint options.
    explicit Starts(const text::String &value, ConstraintOptions options = {}) :
        StringPartConstraint<impl::StartsConstraint>(value, std::move(options)) {
        _name = "starts"_el;
    }
    /// Creates a starts-with constraint from expected prefixes.
    /// @param values The expected prefixes.
    /// @param options Additional constraint options.
    explicit Starts(const std::initializer_list<text::String> values, ConstraintOptions options = {}) :
        StringPartConstraint<impl::StartsConstraint>(values, std::move(options)) {
        _name = "starts"_el;
    }
};

}
