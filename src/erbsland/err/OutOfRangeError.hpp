// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "RuntimeError.hpp"

namespace erbsland::err {

/// An out-of-range error exception.
/// Reports recoverable numeric bounds and configured processing-limit errors.
/// @tested{ExceptionHierarchyTest}
class OutOfRangeError : public RuntimeError {
public:
    /// Create an out-of-range error with a reason.
    /// @param reason The reason for the out-of-range error.
    explicit OutOfRangeError(text::String reason) noexcept : RuntimeError{std::move(reason)} {}
    /// @overload
    explicit OutOfRangeError(const std::string_view reason) noexcept : RuntimeError{reason} {}

    // defaults
    ~OutOfRangeError() override = default;
};

}
