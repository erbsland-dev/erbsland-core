// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Attribute.hpp"

#include "../../../../text/String.hpp"

#include <utility>

namespace erbsland::conf::vr::builder {

/// Sets the default validation error message for a rule.
struct CustomError : Attribute {
    explicit CustomError(text::String errorMessage) : _errorMessage{std::move(errorMessage)} {}
    void operator()(impl::Rule &rule) override;
    text::String _errorMessage;
};

}
