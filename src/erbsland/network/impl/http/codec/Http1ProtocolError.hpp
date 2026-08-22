// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Http1FailureReason.hpp"

#include "../../../../err/RuntimeError.hpp"

namespace erbsland::network::impl {

/// An internal categorized HTTP/1.x protocol failure.
/// @tested{Http1CodecTest}
class Http1ProtocolError final : public err::RuntimeError {
public:
    /// Create a categorized protocol error.
    /// @param reason Stable machine-readable failure category.
    /// @param message Nonsensitive local diagnostic text.
    Http1ProtocolError(Http1FailureReason reason, text::String message) noexcept;

    // defaults
    ~Http1ProtocolError() override = default;

public: // accessors
    /// Get the stable failure category.
    [[nodiscard]] auto reason() const noexcept -> Http1FailureReason { return _reason; }

private:
    Http1FailureReason _reason; ///< Stable failure category.
};

}
