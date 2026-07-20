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
struct Matches : ConstraintAttribute {
    explicit Matches(const text::String &pattern, const bool isVerbose = false, ConstraintOptions options = {}) :
        _pattern{pattern}, _isVerbose{isVerbose}, _options{std::move(options)} {}
    explicit Matches(re::RegExPtr pattern, ConstraintOptions options = {}) :
        _compiledPattern{std::move(pattern)}, _options{std::move(options)} {
        if (_compiledPattern == nullptr) {
            throw err::ParameterError{"The regular expression cannot be null.", "pattern"};
        }
    }

    void operator()(impl::Rule &rule) override;

    text::String _pattern;
    re::RegExPtr _compiledPattern;
    bool _isVerbose{false};
    ConstraintOptions _options;
};

}
