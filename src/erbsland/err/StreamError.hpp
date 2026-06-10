// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "RuntimeError.hpp"

namespace erbsland::err {

/// A stream operation error.
/// These exceptions are thrown when a stream cannot complete a requested read, write, flush, seek, or close operation.
/// @notest{Trivial exception type.}
class StreamError : public RuntimeError {
public:
    /// Create a stream error with a reason.
    /// @param reason The reason for the stream error.
    explicit StreamError(text::StringView reason) noexcept : RuntimeError{std::move(reason)} {}
    /// @overload
    explicit StreamError(const std::string_view reason) noexcept : RuntimeError{reason} {}

    // defaults
    ~StreamError() override = default;
};

}
