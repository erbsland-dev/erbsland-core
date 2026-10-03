// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../err/LogicFailure.hpp"

#include <exception>

namespace erbsland::random {

/// A random number generation error.
/// These exceptions are thrown when a random source cannot provide the requested data.
/// @tested{SecureRandomTest}
class RandomFailure final : public err::LogicFailure {
public:
    /// Create a random error with a reason.
    /// @param reason The reason for the random error.
    explicit RandomFailure(text::String reason) noexcept : err::LogicFailure{std::move(reason)} {}
    /// Create a random error with a reason and diagnostic cause.
    /// @param reason The reason for the random error.
    /// @param cause The diagnostic cause.
    explicit RandomFailure(text::String reason, std::exception_ptr cause) noexcept :
        err::LogicFailure{std::move(reason), std::move(cause)} {}
    /// @overload
    explicit RandomFailure(const std::string_view reason) noexcept : err::LogicFailure{reason} {}
    /// @overload
    explicit RandomFailure(const std::string_view reason, std::exception_ptr cause) noexcept :
        err::LogicFailure{reason, std::move(cause)} {}
};

}
