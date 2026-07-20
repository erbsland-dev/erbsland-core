// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConstraintAttribute.hpp"
#include "ConstraintOptions.hpp"

#include "../TypeTraits.hpp"

#include "../../../../mem/ByteBlock.hpp"

#include <utility>
#include <variant>

namespace erbsland::conf::vr::builder {

using namespace text::literals;

/// Adds an equality constraint for scalar values or matrix size.
struct Equals : ConstraintAttribute {
    using Value = std::variant<Integer, bool, Float, text::String, mem::ByteBlock, std::pair<Integer, Integer>>;

    template <typename TValue>
        requires(IsInteger<TValue>)
    explicit Equals(const TValue value, ConstraintOptions options = {}) :
        _value{static_cast<Integer>(value)}, _options{std::move(options)} {}
    explicit Equals(const bool value, ConstraintOptions options = {}) : _value{value}, _options{std::move(options)} {}
    template <typename TValue>
        requires(IsFloat<TValue>)
    explicit Equals(const TValue value, ConstraintOptions options = {}) :
        _value{static_cast<Float>(value)}, _options{std::move(options)} {}
    explicit Equals(const text::String &value, ConstraintOptions options = {}) :
        _value{value}, _options{std::move(options)} {}
    explicit Equals(const mem::ByteBlock &value, ConstraintOptions options = {}) :
        _value{value}, _options{std::move(options)} {}
    explicit Equals(const std::pair<Integer, Integer> value, ConstraintOptions options = {}) :
        _value{value}, _options{std::move(options)} {}
    Equals(const Integer first, const Integer second, ConstraintOptions options = {}) :
        _value{std::pair<Integer, Integer>{first, second}}, _options{std::move(options)} {}

    void operator()(impl::Rule &rule) override;

    Value _value;
    ConstraintOptions _options;
};

}
