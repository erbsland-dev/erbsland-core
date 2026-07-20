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
    explicit ConfErrorDiagnostic(ConfErrorContext context) noexcept;

public: // implement Diagnostic
    [[nodiscard]] auto sourcePath() const noexcept -> text::String override;
    [[nodiscard]] auto location() const noexcept -> unit::CodeLocation override;
    [[nodiscard]] auto toString() const noexcept -> text::String override;
    [[nodiscard]] auto toTextDocument(const i18n::DisplayTextMapConstPtr &displayText = {}) const
        -> text::TextDocument override;

private:
    ConfErrorContext _context;
};

}
