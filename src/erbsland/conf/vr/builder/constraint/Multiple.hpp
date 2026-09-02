// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConstraintAttribute.hpp"
#include "ConstraintOptions.hpp"

#include "../../../Float.hpp"
#include "../../../Integer.hpp"

#include <type_traits>
#include <utility>
#include <variant>

namespace erbsland::conf::vr::builder {

using namespace text::literals;

/// Adds a multiple-of constraint for scalar values or matrix size.
class Multiple : public ConstraintAttribute {
public:
    using Value = std::variant<Integer, Float, std::pair<Integer, Integer>>;

    /// Creates an integer multiple-of constraint.
    /// @tparam TValue An integer type.
    /// @param value The required divisor.
    /// @param options Additional constraint options.
    template <typename TValue>
        requires(std::is_integral_v<TValue> && !std::is_same_v<TValue, bool>)
    explicit Multiple(const TValue value, ConstraintOptions options = {}) :
        _value{static_cast<Integer>(value)}, _options{std::move(options)} {}
    /// Creates a floating-point multiple-of constraint.
    /// @tparam TValue A floating-point type.
    /// @param value The required divisor.
    /// @param options Additional constraint options.
    template <typename TValue>
        requires std::is_floating_point_v<TValue>
    explicit Multiple(const TValue value, ConstraintOptions options = {}) :
        _value{static_cast<Float>(value)}, _options{std::move(options)} {}
    /// Creates a matrix-dimension multiple-of constraint.
    /// @param value The required row and column divisors.
    /// @param options Additional constraint options.
    explicit Multiple(const std::pair<Integer, Integer> value, ConstraintOptions options = {}) :
        _value{value}, _options{std::move(options)} {}
    /// Creates a matrix-dimension multiple-of constraint.
    /// @param rows The required row divisor.
    /// @param columns The required column divisor.
    /// @param options Additional constraint options.
    Multiple(const Integer rows, const Integer columns, ConstraintOptions options = {}) :
        _value{std::pair<Integer, Integer>{rows, columns}}, _options{std::move(options)} {}

    void apply(RuleDefinition &rule) const override;

    Value _value;
    ConstraintOptions _options;
};

}
