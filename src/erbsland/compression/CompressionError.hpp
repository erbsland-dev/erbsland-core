// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CompressionErrorContext.hpp"

#include "../err/RuntimeError.hpp"

#include <string_view>

namespace erbsland::compression {

/// A malformed or unsupported byte-compression representation.
/// @tested{ByteCompressionTest CompressionErrorTest}
class CompressionError final : public err::RuntimeError {
public:
    /// Create a byte-compression error.
    CompressionError(CompressionErrorReason reason, text::String message) noexcept;
    /// @overload
    CompressionError(CompressionErrorReason reason, std::string_view message) noexcept;
    /// Create a byte-compression error from complete diagnostic context.
    explicit CompressionError(CompressionErrorContext context) noexcept;

    // defaults
    ~CompressionError() override = default;

public: // implement RuntimeError
    /// Create the complete structured diagnostic.
    [[nodiscard]] auto diagnostic() const -> err::DiagnosticConstPtr override;

public: // accessors
    /// Get the complete error context.
    [[nodiscard]] auto context() const noexcept -> const CompressionErrorContext & { return _context; }
    /// Get the machine-readable failure reason.
    [[nodiscard]] auto reasonCode() const noexcept -> CompressionErrorReason { return _context.reason(); }
    /// Get the concise error title.
    [[nodiscard]] auto title() const noexcept -> const text::String & { return _context.title(); }
    /// Get the detailed error description.
    [[nodiscard]] auto description() const noexcept -> const text::String & { return _context.description(); }

private:
    CompressionErrorContext _context; ///< Complete diagnostic context for this error.
};

}
