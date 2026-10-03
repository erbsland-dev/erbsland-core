// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "RuntimeError.hpp"

namespace erbsland::err {

/// A requested key or name was not found.
/// @tested{ExceptionHierarchyTest}
class NotFoundError : public RuntimeError {
public:
    /// Create a lookup error with a reason.
    /// @param reason The reason for the lookup error.
    explicit NotFoundError(text::String reason) noexcept : RuntimeError{std::move(reason)} {}
    /// @overload
    explicit NotFoundError(const std::string_view reason) noexcept : RuntimeError{reason} {}

    // defaults
    ~NotFoundError() override = default;
};

}
