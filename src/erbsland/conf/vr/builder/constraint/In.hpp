// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConstraintAttribute.hpp"
#include "ConstraintOptions.hpp"

#include "../../../../mem/ByteBlock.hpp"
#include "../../../../text/StringList.hpp"
#include "../../../../util/List.hpp"
#include "../../../Float.hpp"
#include "../../../Integer.hpp"

#include <concepts>
#include <type_traits>
#include <utility>
#include <variant>

namespace erbsland::conf::vr::builder {

using namespace text::literals;

/// Adds an inclusion constraint for a list of values.
class In : public ConstraintAttribute {
public:
    using ValueList =
        std::variant<util::List<Integer>, util::List<Float>, text::StringList, util::List<mem::ByteBlock>>;

    /// Creates an integer inclusion constraint.
    /// @param values The allowed integer values.
    /// @param options Additional constraint options.
    explicit In(util::List<Integer> values, ConstraintOptions options = {}) :
        _values{std::move(values)}, _options{std::move(options)} {}
    /// Creates a floating-point inclusion constraint.
    /// @param values The allowed floating-point values.
    /// @param options Additional constraint options.
    explicit In(util::List<Float> values, ConstraintOptions options = {}) :
        _values{std::move(values)}, _options{std::move(options)} {}
    /// Creates a string inclusion constraint.
    /// @param values The allowed strings.
    /// @param options Additional constraint options.
    explicit In(text::StringList values, ConstraintOptions options = {}) :
        _values{std::move(values)}, _options{std::move(options)} {}
    /// Creates a byte-block inclusion constraint.
    /// @param values The allowed byte blocks.
    /// @param options Additional constraint options.
    explicit In(util::List<mem::ByteBlock> values, ConstraintOptions options = {}) :
        _values{std::move(values)}, _options{std::move(options)} {}

    /// Creates an integer inclusion constraint.
    /// @param values The allowed integer values.
    /// @param options Additional constraint options.
    explicit In(const std::initializer_list<Integer> values, ConstraintOptions options = {}) :
        In(util::List<Integer>{values}, std::move(options)) {}
    /// Creates a floating-point inclusion constraint.
    /// @param values The allowed floating-point values.
    /// @param options Additional constraint options.
    explicit In(const std::initializer_list<Float> values, ConstraintOptions options = {}) :
        In(util::List<Float>{values}, std::move(options)) {}
    /// Creates a string inclusion constraint.
    /// @param values The allowed strings.
    /// @param options Additional constraint options.
    explicit In(const std::initializer_list<text::String> values, ConstraintOptions options = {}) :
        In(text::StringList{values}, std::move(options)) {}
    /// Creates a byte-block inclusion constraint.
    /// @param values The allowed byte blocks.
    /// @param options Additional constraint options.
    explicit In(const std::initializer_list<mem::ByteBlock> values, ConstraintOptions options = {}) :
        In(util::List<mem::ByteBlock>{values}, std::move(options)) {}

    /// Creates an integer inclusion constraint.
    /// @param value The allowed integer value.
    /// @param options Additional constraint options.
    template <std::integral tValue>
        requires(std::is_integral_v<tValue> && !std::is_same_v<tValue, bool>)
    explicit In(const tValue value, ConstraintOptions options = {}) :
        In(util::List<Integer>{static_cast<Integer>(value)}, std::move(options)) {}
    /// Creates a floating-point inclusion constraint.
    /// @param value The allowed floating-point value.
    /// @param options Additional constraint options.
    template <std::floating_point tValue>
        requires std::is_floating_point_v<tValue>
    explicit In(const tValue value, ConstraintOptions options = {}) :
        In(util::List<Float>{static_cast<Float>(value)}, std::move(options)) {}
    /// Creates a string inclusion constraint.
    /// @param value The allowed string.
    /// @param options Additional constraint options.
    explicit In(const text::String &value, ConstraintOptions options = {}) :
        In(text::StringList{value}, std::move(options)) {}
    /// Creates a byte-block inclusion constraint.
    /// @param value The allowed byte block.
    /// @param options Additional constraint options.
    explicit In(const mem::ByteBlock &value, ConstraintOptions options = {}) :
        In(util::List<mem::ByteBlock>{value}, std::move(options)) {}

    void apply(RuleDefinition &rule) const override;

private:
    ValueList _values;
    ConstraintOptions _options;
};

}
