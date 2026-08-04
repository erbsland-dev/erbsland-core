// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConstraintAttribute.hpp"
#include "ConstraintOptions.hpp"

#include "../../../../mem/ByteBlock.hpp"
#include "../../../impl/vr/TypeTraits.hpp"

#include <utility>
#include <variant>

namespace erbsland::conf::vr::builder {

using namespace text::literals;

/// Adds an equality constraint for scalar values or matrix size.
class Equals : public ConstraintAttribute {
public:
    using Value = std::variant<Integer, bool, Float, text::String, mem::ByteBlock, std::pair<Integer, Integer>>;

    /// Creates an integer equality constraint.
    /// @tparam TValue An integer type.
    /// @param value The expected value.
    /// @param options Additional constraint options.
    template <typename TValue>
        requires(impl::IsInteger<TValue>)
    explicit Equals(const TValue value, ConstraintOptions options = {}) :
        _value{static_cast<Integer>(value)}, _options{std::move(options)} {}
    /// Creates a Boolean equality constraint.
    /// @param value The expected value.
    /// @param options Additional constraint options.
    explicit Equals(const bool value, ConstraintOptions options = {}) : _value{value}, _options{std::move(options)} {}
    /// Creates a floating-point equality constraint.
    /// @tparam TValue A floating-point type.
    /// @param value The expected value.
    /// @param options Additional constraint options.
    template <typename TValue>
        requires(impl::IsFloat<TValue>)
    explicit Equals(const TValue value, ConstraintOptions options = {}) :
        _value{static_cast<Float>(value)}, _options{std::move(options)} {}
    /// Creates a text equality constraint.
    /// @param value The expected value.
    /// @param options Additional constraint options.
    explicit Equals(const text::String &value, ConstraintOptions options = {}) :
        _value{value}, _options{std::move(options)} {}
    /// Creates a byte-block equality constraint.
    /// @param value The expected value.
    /// @param options Additional constraint options.
    explicit Equals(const mem::ByteBlock &value, ConstraintOptions options = {}) :
        _value{value}, _options{std::move(options)} {}
    /// Creates a matrix-size equality constraint.
    /// @param value The expected row and column counts.
    /// @param options Additional constraint options.
    explicit Equals(const std::pair<Integer, Integer> value, ConstraintOptions options = {}) :
        _value{value}, _options{std::move(options)} {}
    /// Creates a matrix-size equality constraint.
    /// @param first The expected first dimension.
    /// @param second The expected second dimension.
    /// @param options Additional constraint options.
    Equals(const Integer first, const Integer second, ConstraintOptions options = {}) :
        _value{std::pair<Integer, Integer>{first, second}}, _options{std::move(options)} {}

    void operator()(Rule &rule) override;

    Value _value;
    ConstraintOptions _options;
};

}
