// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConstraintAttribute.hpp"
#include "ConstraintOptions.hpp"

#include "../../../../err/ParameterError.hpp"
#include "../../../../re/RegEx_fwd.hpp"

#include <utility>

namespace erbsland::conf::vr::builder {

/// Adds a regular-expression constraint for text values.
class Matches : public ConstraintAttribute {
public:
    /// Creates a regular-expression constraint from a pattern.
    /// @param pattern The regular-expression pattern.
    /// @param isVerbose `true` to enable verbose pattern syntax.
    /// @param options Additional constraint options.
    explicit Matches(const text::String &pattern, const bool isVerbose = false, ConstraintOptions options = {}) :
        _pattern{pattern}, _isVerbose{isVerbose}, _options{std::move(options)} {}
    /// Creates a regular-expression constraint from a compiled expression.
    /// @param pattern The compiled regular expression.
    /// @param options Additional constraint options.
    explicit Matches(re::RegExPtr pattern, ConstraintOptions options = {}) :
        _compiledPattern{std::move(pattern)}, _options{std::move(options)} {
        if (_compiledPattern == nullptr) {
            using namespace text::literals;
            throw err::ParameterError{"The regular expression cannot be null."_el, "pattern"_el};
        }
    }

    void apply(RuleDefinition &rule) const override;

    text::String _pattern;
    re::RegExPtr _compiledPattern;
    bool _isVerbose{false};
    ConstraintOptions _options;
};

}
