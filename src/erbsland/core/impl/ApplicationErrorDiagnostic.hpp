// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../ApplicationErrorContext.hpp"

#include "../../err/Diagnostic.hpp"

namespace erbsland::core::impl {

/// The diagnostic for an application error.
/// @tested{ApplicationErrorTest}
class ApplicationErrorDiagnostic final : public err::Diagnostic {
public:
    /// Create a new diagnostic for an application error.
    explicit ApplicationErrorDiagnostic(ApplicationErrorContext context) noexcept;

public:
    [[nodiscard]] auto sourceName() const noexcept -> text::String override;
    [[nodiscard]] auto sourcePath() const noexcept -> text::String override;
    [[nodiscard]] auto location() const noexcept -> unit::CodeLocation override;
    [[nodiscard]] auto toTextDocument(const i18n::DisplayTextMapConstPtr &displayText) const
        -> text::TextDocument override;

private:
    ApplicationErrorContext _context;
};

}
