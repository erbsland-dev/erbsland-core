// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConstraintAttribute.hpp"
#include "ConstraintOptions.hpp"

#include "../../../../text/StringList.hpp"

#include <utility>

namespace erbsland::conf::vr::builder {

/// Adds a character-set constraint for text values.
class Chars : public ConstraintAttribute {
public:
    /// Creates a character-set constraint from allowed characters.
    /// @param values The allowed characters.
    /// @param options Additional constraint options.
    explicit Chars(text::StringList values, ConstraintOptions options = {}) :
        _values{std::move(values)}, _options{std::move(options)} {}
    /// Creates a character-set constraint from one allowed character.
    /// @param value The allowed character.
    /// @param options Additional constraint options.
    explicit Chars(const text::String &value, ConstraintOptions options = {}) :
        _values{{value}}, _options{std::move(options)} {}
    /// Creates a character-set constraint from allowed characters.
    /// @param values The allowed characters.
    /// @param options Additional constraint options.
    explicit Chars(const std::initializer_list<text::String> values, ConstraintOptions options = {}) :
        _values{values}, _options{std::move(options)} {}

    void operator()(Rule &rule) override;

private:
    text::StringList _values;
    ConstraintOptions _options;
};

}
