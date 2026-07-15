// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../err/LogicError.hpp"

#include <exception>

namespace erbsland::random {

/// A random number generation error.
/// These exceptions are thrown when a random source cannot provide the requested data.
/// @tested{SecureRandomTest}
class RandomError final : public err::LogicError {
public:
    /// Create a random error with a reason.
    /// @param reason The reason for the random error.
    explicit RandomError(text::StringView reason) noexcept : err::LogicError{std::move(reason)} {}
    /// Create a random error with a reason and diagnostic cause.
    /// @param reason The reason for the random error.
    /// @param cause The diagnostic cause.
    explicit RandomError(text::StringView reason, std::exception_ptr cause) noexcept :
        err::LogicError{std::move(reason), std::move(cause)} {}
    /// @overload
    explicit RandomError(const std::string_view reason) noexcept : err::LogicError{reason} {}
    /// @overload
    explicit RandomError(const std::string_view reason, std::exception_ptr cause) noexcept :
        err::LogicError{reason, std::move(cause)} {}
};

}
