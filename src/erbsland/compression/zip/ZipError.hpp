// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ZipErrorContext.hpp"

#include "../../err/RuntimeError.hpp"

#include <exception>

namespace erbsland::compression::zip {

/// A malformed, unsupported, unsafe, or failed ZIP archive operation.
/// @tested{ZipArchiveTest}
class ZipError final : public err::RuntimeError {
public:
    /// Create a ZIP error and retain an optional lower-level cause.
    explicit ZipError(ZipErrorContext context, std::exception_ptr cause = {}) noexcept;
    // defaults
    ~ZipError() override = default;

public:
    /// Build a user-facing diagnostic containing all available path context.
    [[nodiscard]] auto diagnostic() const -> err::DiagnosticConstPtr override;
    /// Get the complete structured error context.
    [[nodiscard]] auto context() const noexcept -> const ZipErrorContext & { return _context; }
    /// Get the machine-readable failure reason.
    [[nodiscard]] auto reasonCode() const noexcept -> ZipErrorReason { return _context.reason(); }
    /// Get the operation phase that failed.
    [[nodiscard]] auto phase() const noexcept -> ZipOperationPhase { return _context.phase(); }
    /// Get the user-facing failure title.
    [[nodiscard]] auto title() const noexcept -> const text::String & { return _context.title(); }
    /// Get the user-facing failure description.
    [[nodiscard]] auto description() const noexcept -> const text::String & { return _context.description(); }

private:
    ZipErrorContext _context;
};

}
