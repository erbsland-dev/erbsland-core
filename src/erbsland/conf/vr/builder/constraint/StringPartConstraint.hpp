// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConstraintAttribute.hpp"
#include "ConstraintOptions.hpp"

#include "../../../../text/StringList.hpp"

#include <utility>

namespace erbsland::conf::vr::builder {

using namespace text::literals;

/// Shared value storage for string-part constraint attributes.
/// @tested{VrBuilderApiTest}
class StringPartConstraint : public ConstraintAttribute {
protected:
    /// Identifies the concrete string-part operation.
    enum class Kind {
        Starts,   ///< A starts-with constraint.
        Ends,     ///< An ends-with constraint.
        Contains, ///< A contains constraint.
    };

protected:
    /// Creates a string-part constraint from expected text parts.
    /// @param kind The kind of string-part constraint.
    /// @param values The expected text parts.
    /// @param options Additional constraint options.
    explicit StringPartConstraint(Kind kind, text::StringList values, ConstraintOptions options = {}) :
        _kind{kind}, _values{std::move(values)}, _options{std::move(options)} {}

    /// Creates a string-part constraint from one expected text part.
    /// @param kind The kind of string-part constraint.
    /// @param value The expected text part.
    /// @param options Additional constraint options.
    explicit StringPartConstraint(Kind kind, const text::String &value, ConstraintOptions options = {}) :
        StringPartConstraint{kind, text::StringList{{value}}, std::move(options)} {}
    /// Creates a string-part constraint from expected text parts.
    /// @param kind The kind of string-part constraint.
    /// @param values The expected text parts.
    /// @param options Additional constraint options.
    explicit StringPartConstraint(
        Kind kind, const std::initializer_list<text::String> values, ConstraintOptions options = {}) :
        StringPartConstraint{kind, text::StringList{values}, std::move(options)} {}

public: // implement Attribute
    void apply(RuleDefinition &rule) const override;

private:
    Kind _kind; ///< The string-part operation.
    text::StringList _values;
    ConstraintOptions _options;
};

}
