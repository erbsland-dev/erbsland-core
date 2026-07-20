// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringPartConstraint.hpp"

#include "../../../impl/vr/StringPartConstraint.hpp"

#include <utility>

namespace erbsland::conf::vr::builder {

using namespace text::literals;

/// Adds a starts-with text constraint.
struct Starts final : StringPartConstraint<impl::StartsConstraint> {
    explicit Starts(text::StringList values, ConstraintOptions options = {}) :
        StringPartConstraint<impl::StartsConstraint>(std::move(values), std::move(options)) {
        _name = "starts"_el;
    }
    explicit Starts(const text::String &value, ConstraintOptions options = {}) :
        StringPartConstraint<impl::StartsConstraint>(value, std::move(options)) {
        _name = "starts"_el;
    }
    explicit Starts(const std::initializer_list<text::String> values, ConstraintOptions options = {}) :
        StringPartConstraint<impl::StartsConstraint>(values, std::move(options)) {
        _name = "starts"_el;
    }
};

}
