// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PlatformErrorContext_fwd.hpp"

#include "../err/LogicFailure.hpp"

namespace erbsland::system {

/// A fatal native platform failure with immutable diagnostic context.
/// @tested{DiagnosticTest ExceptionHierarchyTest NativeEventLoopFailureTest}
class PlatformFailure final : public err::LogicFailure {
public:
    /// Create a fatal native platform failure.
    /// @param reason A developer-authored description of the failed operation.
    /// @param context The captured native diagnostic context.
    /// @param cause An optional independent cause.
    explicit PlatformFailure(
        text::String reason, PlatformErrorContextConstPtr context = {}, std::exception_ptr cause = {}) noexcept;

    // defaults
    ~PlatformFailure() override = default;

public: // implement Exception
    [[nodiscard]] auto diagnostic() const -> err::DiagnosticConstPtr override;

public: // accessors
    /// Get the captured native diagnostic context.
    [[nodiscard]] auto context() const noexcept -> const PlatformErrorContextConstPtr & { return _context; }

private:
    PlatformErrorContextConstPtr _context; ///< Captured native diagnostic context.
};

}
