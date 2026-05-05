// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Exception.hpp"

namespace erbsland::err {

/// An overflow error exception.
/// These exceptions are thrown when an operation results in an overflow.
class OverflowError : public Exception {
public:
    /// Create an overflow error exception with a reason.
    /// @param reason The reason for the overflow error.
    explicit OverflowError(text::StringView reason) noexcept : Exception{std::move(reason)} {}
    /// @overload
    explicit OverflowError(const std::string_view reason) noexcept : Exception{reason} {}
};

}
