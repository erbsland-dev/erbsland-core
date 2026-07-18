// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LogicError.hpp"

namespace erbsland::err {

/// A parameter error exception.
/// These exceptions are thrown when you pass invalid parameters to a function.
/// Usually not meant to be caught by the application.
class ParameterError : public LogicError {
public:
    /// Create an overflow error exception with a reason.
    /// @param reason The reason for the overflow error.
    /// @param parameterName The name of the parameter that caused the error.
    explicit ParameterError(text::String reason, text::String parameterName) noexcept;
    /// @overload
    explicit ParameterError(std::string_view reason, std::string_view parameterName) noexcept;

    // defaults
    ~ParameterError() override = default;

public:
    [[nodiscard]] auto toString() const noexcept -> text::String override;

private:
    text::String _parameterName; ///< The name of the parameter that caused the error.
};

}
