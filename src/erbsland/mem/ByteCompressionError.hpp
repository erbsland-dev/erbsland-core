// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ByteCompressionErrorReason.hpp"

#include "../err/RuntimeError.hpp"

namespace erbsland::mem {

/// A malformed or unsupported byte-compression representation.
/// @tested{ByteCompressionTest}
class ByteCompressionError final : public err::RuntimeError {
public:
    /// Create a byte-compression error.
    ByteCompressionError(ByteCompressionErrorReason reason, text::String message) noexcept;
    /// @overload
    ByteCompressionError(ByteCompressionErrorReason reason, std::string_view message) noexcept;

    // defaults
    ~ByteCompressionError() override = default;

public:
    /// Get the machine-readable failure reason.
    [[nodiscard]] auto reasonCode() const noexcept -> ByteCompressionErrorReason { return _reason; }

private:
    ByteCompressionErrorReason _reason; ///< The machine-readable failure reason.
};

}
