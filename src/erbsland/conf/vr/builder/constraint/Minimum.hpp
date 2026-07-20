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

/// Adds a minimum boundary constraint for numeric or temporal values.
struct Minimum : ConstraintAttribute {
    using Value = std::variant<Integer, Float, time::Date, time::DateTime, std::pair<Integer, Integer>>;

    template <typename TValue>
        requires(IsInteger<TValue>)
    explicit Minimum(const TValue value, ConstraintOptions options = {}) :
        _value{static_cast<Integer>(value)}, _options{std::move(options)} {}
    template <typename TValue>
        requires(IsFloat<TValue>)
    explicit Minimum(const TValue value, ConstraintOptions options = {}) :
        _value{static_cast<Float>(value)}, _options{std::move(options)} {}
    explicit Minimum(const time::Date &value, ConstraintOptions options = {}) :
        _value{value}, _options{std::move(options)} {}
    explicit Minimum(const time::DateTime &value, ConstraintOptions options = {}) :
        _value{value}, _options{std::move(options)} {}
    explicit Minimum(const std::pair<Integer, Integer> value, ConstraintOptions options = {}) :
        _value{value}, _options{std::move(options)} {}
    Minimum(const Integer first, const Integer second, ConstraintOptions options = {}) :
        _value{std::pair<Integer, Integer>{first, second}}, _options{std::move(options)} {}

    void operator()(impl::Rule &rule) override;

    Value _value;
    ConstraintOptions _options;
};

}
