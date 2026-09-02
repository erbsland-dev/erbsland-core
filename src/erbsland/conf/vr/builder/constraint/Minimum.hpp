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

/// Adds a minimum boundary constraint for numeric or temporal values.
class Minimum : public ConstraintAttribute {
public:
    using Value = std::variant<Integer, Float, time::Date, time::DateTime, std::pair<Integer, Integer>>;

    /// Creates an integer minimum constraint.
    /// @tparam TValue An integer type.
    /// @param value The minimum value.
    /// @param options Additional constraint options.
    template <typename TValue>
        requires(std::is_integral_v<TValue> && !std::is_same_v<TValue, bool>)
    explicit Minimum(const TValue value, ConstraintOptions options = {}) :
        _value{static_cast<Integer>(value)}, _options{std::move(options)} {}
    /// Creates a floating-point minimum constraint.
    /// @tparam TValue A floating-point type.
    /// @param value The minimum value.
    /// @param options Additional constraint options.
    template <typename TValue>
        requires std::is_floating_point_v<TValue>
    explicit Minimum(const TValue value, ConstraintOptions options = {}) :
        _value{static_cast<Float>(value)}, _options{std::move(options)} {}
    /// Creates a date minimum constraint.
    /// @param value The minimum date.
    /// @param options Additional constraint options.
    explicit Minimum(const time::Date &value, ConstraintOptions options = {}) :
        _value{value}, _options{std::move(options)} {}
    /// Creates a date-time minimum constraint.
    /// @param value The minimum date and time.
    /// @param options Additional constraint options.
    explicit Minimum(const time::DateTime &value, ConstraintOptions options = {}) :
        _value{value}, _options{std::move(options)} {}
    /// Creates a matrix-size minimum constraint.
    /// @param value The minimum row and column counts.
    /// @param options Additional constraint options.
    explicit Minimum(const std::pair<Integer, Integer> value, ConstraintOptions options = {}) :
        _value{value}, _options{std::move(options)} {}
    /// Creates a matrix-size minimum constraint.
    /// @param first The minimum first dimension.
    /// @param second The minimum second dimension.
    /// @param options Additional constraint options.
    Minimum(const Integer first, const Integer second, ConstraintOptions options = {}) :
        _value{std::pair<Integer, Integer>{first, second}}, _options{std::move(options)} {}

    void apply(RuleDefinition &rule) const override;

    Value _value;
    ConstraintOptions _options;
};

}
