// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Attribute.hpp"

#include "../../../../text/String.hpp"

#include <utility>

namespace erbsland::conf::vr::builder {

/// Sets the default validation error message for a rule.
class CustomError : public Attribute {
public:
    /// Set a custom validation error message.
    /// @param errorMessage The message to move into the attribute.
    explicit CustomError(text::String errorMessage) : _errorMessage{std::move(errorMessage)} {}
    void operator()(Rule &rule) override;
    text::String _errorMessage;
};

}
