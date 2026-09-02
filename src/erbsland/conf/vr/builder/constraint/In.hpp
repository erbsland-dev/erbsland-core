// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConstraintAttribute.hpp"
#include "ConstraintOptions.hpp"

#include "../../../../mem/ByteBlock.hpp"
#include "../../../../text/StringList.hpp"
#include "../../../Float.hpp"
#include "../../../Integer.hpp"

#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace erbsland::conf::vr::builder {

using namespace text::literals;

/// Adds an inclusion constraint for a list of values.
class In : public ConstraintAttribute {
public:
    using ValueList =
        std::variant<std::vector<Integer>, std::vector<Float>, text::StringList, std::vector<mem::ByteBlock>>;

    /// Creates an integer inclusion constraint.
    /// @param values The allowed integer values.
    /// @param options Additional constraint options.
    explicit In(std::vector<Integer> values, ConstraintOptions options = {}) :
        _values{std::move(values)}, _options{std::move(options)} {}
    /// Creates a floating-point inclusion constraint.
    /// @param values The allowed floating-point values.
    /// @param options Additional constraint options.
    explicit In(std::vector<Float> values, ConstraintOptions options = {}) :
        _values{std::move(values)}, _options{std::move(options)} {}
    /// Creates a string inclusion constraint.
    /// @param values The allowed strings.
    /// @param options Additional constraint options.
    explicit In(text::StringList values, ConstraintOptions options = {}) :
        _values{std::move(values)}, _options{std::move(options)} {}
    /// Creates a byte-block inclusion constraint.
    /// @param values The allowed byte blocks.
    /// @param options Additional constraint options.
    explicit In(std::vector<mem::ByteBlock> values, ConstraintOptions options = {}) :
        _values{std::move(values)}, _options{std::move(options)} {}

    /// Creates an integer inclusion constraint.
    /// @param values The allowed integer values.
    /// @param options Additional constraint options.
    explicit In(const std::initializer_list<Integer> values, ConstraintOptions options = {}) :
        In(std::vector<Integer>{values}, std::move(options)) {}
    /// Creates a floating-point inclusion constraint.
    /// @param values The allowed floating-point values.
    /// @param options Additional constraint options.
    explicit In(const std::initializer_list<Float> values, ConstraintOptions options = {}) :
        In(std::vector<Float>{values}, std::move(options)) {}
    /// Creates a string inclusion constraint.
    /// @param values The allowed strings.
    /// @param options Additional constraint options.
    explicit In(const std::initializer_list<text::String> values, ConstraintOptions options = {}) :
        In(text::StringList{values}, std::move(options)) {}
    /// Creates a byte-block inclusion constraint.
    /// @param values The allowed byte blocks.
    /// @param options Additional constraint options.
    explicit In(const std::initializer_list<mem::ByteBlock> values, ConstraintOptions options = {}) :
        In(std::vector<mem::ByteBlock>{values}, std::move(options)) {}

    /// Creates an integer inclusion constraint.
    /// @param value The allowed integer value.
    /// @param options Additional constraint options.
    template <typename TValue>
        requires(std::is_integral_v<TValue> && !std::is_same_v<TValue, bool>)
    explicit In(const TValue value, ConstraintOptions options = {}) :
        In(std::vector<Integer>{static_cast<Integer>(value)}, std::move(options)) {}
    /// Creates a floating-point inclusion constraint.
    /// @param value The allowed floating-point value.
    /// @param options Additional constraint options.
    template <typename TValue>
        requires std::is_floating_point_v<TValue>
    explicit In(const TValue value, ConstraintOptions options = {}) :
        In(std::vector<Float>{static_cast<Float>(value)}, std::move(options)) {}
    /// Creates a string inclusion constraint.
    /// @param value The allowed string.
    /// @param options Additional constraint options.
    explicit In(const text::String &value, ConstraintOptions options = {}) :
        In(text::StringList{value}, std::move(options)) {}
    /// Creates a byte-block inclusion constraint.
    /// @param value The allowed byte block.
    /// @param options Additional constraint options.
    explicit In(const mem::ByteBlock &value, ConstraintOptions options = {}) :
        In(std::vector<mem::ByteBlock>{value}, std::move(options)) {}

    void apply(RuleDefinition &rule) const override;

private:
    ValueList _values;
    ConstraintOptions _options;
};

}
