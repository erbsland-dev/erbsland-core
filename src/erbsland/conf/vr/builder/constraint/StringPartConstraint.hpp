// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConstraintAttribute.hpp"
#include "ConstraintOptions.hpp"

#include "../../../../text/StringFormat.hpp"
#include "../../../../text/StringList.hpp"

#include <utility>

namespace erbsland::conf::vr::builder {

using namespace text::literals;

/// Internal helper base for string-part constraints.
template <typename TImplConstraint>
class StringPartConstraint : public ConstraintAttribute {
public:
    /// Creates a string-part constraint from expected text parts.
    /// @param values The expected text parts.
    /// @param options Additional constraint options.
    explicit StringPartConstraint(text::StringList values, ConstraintOptions options = {}) :
        _values{std::move(values)}, _options{std::move(options)} {}

    /// Creates a string-part constraint from one expected text part.
    /// @param value The expected text part.
    /// @param options Additional constraint options.
    explicit StringPartConstraint(const text::String &value, ConstraintOptions options = {}) :
        _values{{value}}, _options{std::move(options)} {}
    /// Creates a string-part constraint from expected text parts.
    /// @param values The expected text parts.
    /// @param options Additional constraint options.
    explicit StringPartConstraint(const std::initializer_list<text::String> values, ConstraintOptions options = {}) :
        _values{values}, _options{std::move(options)} {}

    void operator()(Rule &rule) override {
        requireRuleTypeForConstraint(rule, _name, {vr::RuleType::Text});
        if (_values.isEmpty()) {
            throwValidationError(
                text::StringFormat{"The '{}' constraint must specify a single text value or a list of texts"_el}.build(
                    _name));
        }
        auto constraint = std::make_shared<TImplConstraint>(_values);
        _options.addToRule(rule, constraint, _name);
    }

protected:
    text::StringList _values;
    ConstraintOptions _options;
    text::String _name;
};

}
