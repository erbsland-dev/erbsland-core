// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Exception.hpp"

#include <string_view>
#include <utility>

namespace erbsland::err {

/// A format string or format operation error.
/// These exceptions are thrown when a format pattern is invalid, a format argument does not match the requested
/// field, or formatting would exceed configured safety limits.
/// @tested{U8FormatTest}
class FormatError final : public Exception {
public:
    /// Create a format error with a reason.
    /// @param reason The reason for the format error.
    explicit FormatError(text::StringView reason) noexcept : Exception{std::move(reason)} {}
    /// @overload
    explicit FormatError(const std::string_view reason) noexcept : Exception{reason} {}
};

}
