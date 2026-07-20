// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConstraintAttribute.hpp"
#include "ConstraintOptions.hpp"

#include "../TypeTraits.hpp"

#include <utility>
#include <variant>

namespace erbsland::conf::vr::builder {

using namespace text::literals;

/// Adds a multiple-of constraint for scalar values or matrix size.
struct Multiple : ConstraintAttribute {
    using Value = std::variant<Integer, Float, std::pair<Integer, Integer>>;

    template <typename TValue>
        requires(IsInteger<TValue>)
    explicit Multiple(const TValue value, ConstraintOptions options = {}) :
        _value{static_cast<Integer>(value)}, _options{std::move(options)} {}
    template <typename TValue>
        requires(IsFloat<TValue>)
    explicit Multiple(const TValue value, ConstraintOptions options = {}) :
        _value{static_cast<Float>(value)}, _options{std::move(options)} {}
    explicit Multiple(const std::pair<Integer, Integer> value, ConstraintOptions options = {}) :
        _value{value}, _options{std::move(options)} {}
    Multiple(const Integer rows, const Integer columns, ConstraintOptions options = {}) :
        _value{std::pair<Integer, Integer>{rows, columns}}, _options{std::move(options)} {}

    void operator()(impl::Rule &rule) override;

    Value _value;
    ConstraintOptions _options;
};

}
