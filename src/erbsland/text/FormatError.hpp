// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../err/RuntimeError.hpp"

#include <string_view>
#include <utility>

namespace erbsland::text {

/// A format string or format operation error.
/// These exceptions are thrown when a format pattern is invalid, a format argument does not match the requested
/// field, or formatting would exceed configured safety limits.
/// @tested{U8FormatTest}
class FormatError final : public err::RuntimeError {
public:
    /// Create a format error with a reason.
    /// @param reason The reason for the format error.
    explicit FormatError(StringView reason) noexcept : err::RuntimeError{std::move(reason)} {}
    /// @overload
    explicit FormatError(const std::string_view reason) noexcept : err::RuntimeError{reason} {}

    // defaults
    ~FormatError() override = default;
};

}
