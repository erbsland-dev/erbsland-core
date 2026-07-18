// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../err/Diagnostic.hpp"
#include "../../RegExErrorContext.hpp"

namespace erbsland::re::impl {

/// Diagnostic document for a regular-expression error.
/// @tested{RegExErrorTest}
class RegExErrorDiagnostic final : public err::Diagnostic {
public:
    /// Create a diagnostic from regular-expression error context.
    explicit RegExErrorDiagnostic(RegExErrorContext context) noexcept;

public: // implement Diagnostic
    [[nodiscard]] auto location() const noexcept -> unit::CodeLocation override;
    [[nodiscard]] auto toString() const noexcept -> text::String override;
    [[nodiscard]] auto toTextDocument(const i18n::DisplayTextMapConstPtr &displayText) const
        -> text::TextDocument override;

private:
    RegExErrorContext _context;
};

}
