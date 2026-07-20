// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringPartConstraint.hpp"

#include "../../../impl/vr/StringPartConstraint.hpp"

#include <utility>

namespace erbsland::conf::vr::builder {

using namespace text::literals;

/// Adds an ends-with text constraint.
struct Ends final : StringPartConstraint<impl::EndsConstraint> {
    explicit Ends(text::StringList values, ConstraintOptions options = {}) :
        StringPartConstraint<impl::EndsConstraint>(std::move(values), std::move(options)) {
        _name = "ends"_el;
    }
    explicit Ends(const text::String &value, ConstraintOptions options = {}) :
        StringPartConstraint<impl::EndsConstraint>(value, std::move(options)) {
        _name = "ends"_el;
    }
    explicit Ends(const std::initializer_list<text::String> values, ConstraintOptions options = {}) :
        StringPartConstraint<impl::EndsConstraint>(values, std::move(options)) {
        _name = "ends"_el;
    }
};

}
