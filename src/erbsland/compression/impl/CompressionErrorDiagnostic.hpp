// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../CompressionErrorContext.hpp"

#include "../../err/Diagnostic.hpp"

namespace erbsland::compression::impl {

/// Diagnostic document for a byte-compression error.
/// @tested{CompressionErrorTest}
class CompressionErrorDiagnostic final : public err::Diagnostic {
public:
    /// Create a diagnostic from byte-compression error context.
    /// @param context The context to render.
    explicit CompressionErrorDiagnostic(CompressionErrorContext context) noexcept;

public: // implement Diagnostic
    [[nodiscard]] auto toString() const noexcept -> text::String override;
    [[nodiscard]] auto toTextDocument(const i18n::DisplayTextMapConstPtr &displayText) const
        -> text::TextDocument override;

private:
    /// Create a human-readable name for a structured failure reason.
    [[nodiscard]] static auto reasonName(CompressionErrorReason reason) noexcept -> text::String;

private:
    CompressionErrorContext _context; ///< Compression error context rendered by this diagnostic.
};

}
