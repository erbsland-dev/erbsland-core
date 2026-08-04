// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PlatformErrorCategory.hpp"
#include "PlatformErrorContext_fwd.hpp"

#include "../text/String.hpp"
#include "../text/TextDocument_fwd.hpp"

namespace erbsland::system {

/// Immutable diagnostic details captured from a native operating-system error.
/// @tested{DiagnosticTest}
class PlatformErrorContext {
public:
    // defaults/deletions
    virtual ~PlatformErrorContext() = default;
    PlatformErrorContext(const PlatformErrorContext &) = delete;
    PlatformErrorContext(PlatformErrorContext &&) = delete;
    auto operator=(const PlatformErrorContext &) -> PlatformErrorContext & = delete;
    auto operator=(PlatformErrorContext &&) -> PlatformErrorContext & = delete;

public:
    /// Get the platform-neutral error category.
    [[nodiscard]] virtual auto category() const noexcept -> PlatformErrorCategory = 0;

public: // conversion
    /// Get the native error message or a compact code representation.
    [[nodiscard]] virtual auto toString() const noexcept -> text::String = 0;
    /// Convert the native details into a field-only diagnostic document.
    [[nodiscard]] virtual auto toTextDocument() const -> text::TextDocument = 0;

protected:
    /// Create an empty platform error context.
    PlatformErrorContext() = default;
};

}
