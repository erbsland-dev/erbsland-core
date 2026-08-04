// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../OptionErrorContext.hpp"

#include "../../err/Diagnostic.hpp"
#include "../../text/TextDocument.hpp"

namespace erbsland::options::impl {

/// Diagnostic document for option processing errors.
/// @tested{OptionDocumentTest}
class OptionErrorDiagnostic final : public err::Diagnostic {
public:
    /// Create an option error diagnostic.
    /// @param context The option error context.
    explicit OptionErrorDiagnostic(OptionErrorContext context);

public: // implement Diagnostic
    /// Get the diagnostic source name.
    [[nodiscard]] auto sourceName() const noexcept -> text::String override;
    /// Get the source location of the option error.
    [[nodiscard]] auto location() const noexcept -> unit::CodeLocation override;
    /// Convert the diagnostic to plain text.
    [[nodiscard]] auto toString() const noexcept -> text::String override;
    /// Convert the diagnostic to a structured text document.
    [[nodiscard]] auto toTextDocument(const i18n::DisplayTextMapConstPtr &displayText = {}) const
        -> text::TextDocument override;

private:
    OptionErrorContext _context; ///< The option error context.
};

}
