// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../err/RuntimeError.hpp"

namespace erbsland::text {

/// An encoding error exception.
/// These exceptions are thrown when a text contains encoding errors, and error handing via exception is requested.
class EncodingError : public err::RuntimeError {
public:
    /// Create an encoding error exception with a reason.
    /// @param reason The reason for the encoding error.
    explicit EncodingError(String reason) noexcept : err::RuntimeError{std::move(reason)} {}
    /// @overload
    explicit EncodingError(const std::string_view reason) noexcept : err::RuntimeError{reason} {}

    // defaults
    ~EncodingError() override = default;
};

}
