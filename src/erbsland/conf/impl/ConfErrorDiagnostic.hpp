// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../ConfErrorContext.hpp"

#include "../../err/Diagnostic.hpp"
#include "../../text/TextDocument.hpp"

namespace erbsland::conf::impl {

/// Structured diagnostic for a configuration error.
/// @tested{ConfErrorTest}
class ConfErrorDiagnostic final : public err::Diagnostic {
public:
    /// Create a structured diagnostic from configuration error context.
    /// @param context The error context to retain.
    explicit ConfErrorDiagnostic(ConfErrorContext context) noexcept;

public: // implement Diagnostic
    [[nodiscard]] auto sourcePath() const noexcept -> text::String override;
    [[nodiscard]] auto location() const noexcept -> unit::CodeLocation override;
    [[nodiscard]] auto toString() const noexcept -> text::String override;
    /// Convert this diagnostic into localized structured text.
    /// @param displayText The optional map of localized display texts.
    /// @return The structured diagnostic text.
    [[nodiscard]] auto toTextDocument(const i18n::DisplayTextMapConstPtr &displayText = {}) const
        -> text::TextDocument override;

private:
    ConfErrorContext _context;
};

}
