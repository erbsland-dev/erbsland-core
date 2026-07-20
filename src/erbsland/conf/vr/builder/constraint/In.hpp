// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConstraintAttribute.hpp"
#include "ConstraintOptions.hpp"

#include "../TypeTraits.hpp"

#include "../../../../mem/ByteBlock.hpp"
#include "../../../../text/StringList.hpp"

#include <utility>
#include <variant>
#include <vector>

namespace erbsland::conf::vr::builder {

using namespace text::literals;

/// Adds an inclusion constraint for a list of values.
struct In : ConstraintAttribute {
    using ValueList =
        std::variant<std::vector<Integer>, std::vector<Float>, text::StringList, std::vector<mem::ByteBlock>>;

    explicit In(std::vector<Integer> values, ConstraintOptions options = {}) :
        _values{std::move(values)}, _options{std::move(options)} {}
    explicit In(std::vector<Float> values, ConstraintOptions options = {}) :
        _values{std::move(values)}, _options{std::move(options)} {}
    explicit In(text::StringList values, ConstraintOptions options = {}) :
        _values{std::move(values)}, _options{std::move(options)} {}
    explicit In(std::vector<mem::ByteBlock> values, ConstraintOptions options = {}) :
        _values{std::move(values)}, _options{std::move(options)} {}

    explicit In(const std::initializer_list<Integer> values, ConstraintOptions options = {}) :
        In(std::vector<Integer>{values}, std::move(options)) {}
    explicit In(const std::initializer_list<Float> values, ConstraintOptions options = {}) :
        In(std::vector<Float>{values}, std::move(options)) {}
    explicit In(const std::initializer_list<text::String> values, ConstraintOptions options = {}) :
        In(text::StringList{values}, std::move(options)) {}
    explicit In(const std::initializer_list<mem::ByteBlock> values, ConstraintOptions options = {}) :
        In(std::vector<mem::ByteBlock>{values}, std::move(options)) {}

    template <typename TValue>
        requires(IsInteger<TValue>)
    explicit In(const TValue value, ConstraintOptions options = {}) :
        In(std::vector<Integer>{static_cast<Integer>(value)}, std::move(options)) {}
    template <typename TValue>
        requires(IsFloat<TValue>)
    explicit In(const TValue value, ConstraintOptions options = {}) :
        In(std::vector<Float>{static_cast<Float>(value)}, std::move(options)) {}
    explicit In(const text::String &value, ConstraintOptions options = {}) :
        In(text::StringList{value}, std::move(options)) {}
    explicit In(const mem::ByteBlock &value, ConstraintOptions options = {}) :
        In(std::vector<mem::ByteBlock>{value}, std::move(options)) {}

    void operator()(impl::Rule &rule) override;

private:
    ValueList _values;
    ConstraintOptions _options;
};

}
