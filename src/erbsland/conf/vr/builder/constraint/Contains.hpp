// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringPartConstraint.hpp"

#include "../../../impl/vr/StringPartConstraint.hpp"

#include <utility>

namespace erbsland::conf::vr::builder {

using namespace text::literals;

/// Adds a contains text constraint.
struct Contains final : StringPartConstraint<impl::ContainsConstraint> {
    explicit Contains(text::StringList values, ConstraintOptions options = {}) :
        StringPartConstraint<impl::ContainsConstraint>(std::move(values), std::move(options)) {
        _name = "contains"_el;
    }
    explicit Contains(const text::String &value, ConstraintOptions options = {}) :
        StringPartConstraint<impl::ContainsConstraint>(value, std::move(options)) {
        _name = "contains"_el;
    }
    explicit Contains(const std::initializer_list<text::String> values, ConstraintOptions options = {}) :
        StringPartConstraint<impl::ContainsConstraint>(values, std::move(options)) {
        _name = "contains"_el;
    }
};

}
