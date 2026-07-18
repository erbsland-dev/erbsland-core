// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LogicError.hpp"

namespace erbsland::err {

/// An out-of-range error exception.
/// These exceptions are thrown when an operation results in an out-of-range error.
class OutOfRangeError : public LogicError {
public:
    /// Create an out-of-range error with a reason.
    /// @param reason The reason for the out-of-range error.
    explicit OutOfRangeError(text::String reason) noexcept : LogicError{std::move(reason)} {}
    /// @overload
    explicit OutOfRangeError(const std::string_view reason) noexcept : LogicError{reason} {}

    // defaults
    ~OutOfRangeError() override = default;
};

}
