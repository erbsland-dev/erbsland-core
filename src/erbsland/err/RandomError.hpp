// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LogicError.hpp"

namespace erbsland::err {

/// A random number generation error.
/// These exceptions are thrown when a random source cannot provide the requested data.
/// @tested{SecureRandomTest}
class RandomError final : public LogicError {
public:
    /// Create a random error with a reason.
    /// @param reason The reason for the random error.
    explicit RandomError(text::StringView reason) noexcept : LogicError{std::move(reason)} {}
    /// @overload
    explicit RandomError(const std::string_view reason) noexcept : LogicError{reason} {}
};

}
