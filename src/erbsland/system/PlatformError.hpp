// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PlatformErrorContext_fwd.hpp"

#include "../err/RuntimeError.hpp"

#include <string_view>
#include <utility>

namespace erbsland::system {

/// A native platform error with immutable diagnostic context.
/// @tested{DiagnosticTest}
class PlatformError final : public err::RuntimeError {
public:
    /// Create a native platform error.
    /// @param reason A developer-authored description of the failed operation.
    /// @param context The captured native diagnostic context.
    /// @param cause An optional independent cause.
    explicit PlatformError(
        text::String reason, PlatformErrorContextConstPtr context = {}, std::exception_ptr cause = {}) noexcept;

    // defaults
    ~PlatformError() override = default;

public: // overrides
    [[nodiscard]] auto diagnostic() const -> err::DiagnosticConstPtr override;

public: // accessors
    /// Get the captured native diagnostic context.
    [[nodiscard]] auto context() const noexcept -> const PlatformErrorContextConstPtr & { return _context; }

private:
    PlatformErrorContextConstPtr _context;
};

}
