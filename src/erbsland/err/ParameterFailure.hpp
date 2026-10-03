// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LogicFailure.hpp"

namespace erbsland::err {

/// A caller violation of a parameter precondition.
/// These exceptions are thrown when you pass invalid parameters to a function.
/// Usually not meant to be caught by the application.
/// @tested{ExceptionHierarchyTest}
class ParameterFailure : public LogicFailure {
public:
    /// Create a parameter failure with a reason.
    /// @param reason The reason for the parameter failure.
    /// @param parameterName The name of the parameter that caused the error.
    explicit ParameterFailure(text::String reason, text::String parameterName) noexcept;
    /// @overload
    explicit ParameterFailure(std::string_view reason, std::string_view parameterName) noexcept;

    // defaults
    ~ParameterFailure() override = default;

public:
    [[nodiscard]] auto toString() const noexcept -> text::String override;

private:
    text::String _parameterName; ///< The name of the parameter that caused the error.
};

}
