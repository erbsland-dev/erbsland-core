// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../PathErrorContext.hpp"

#include "../../err/Diagnostic.hpp"
#include "../../text/TextNode_fwd.hpp"

namespace erbsland::path::impl {

/// Diagnostic document for a path error.
/// @tested{DiagnosticTest}
class PathErrorDiagnostic final : public err::Diagnostic {
public:
    /// Create a diagnostic for a path error context.
    explicit PathErrorDiagnostic(PathErrorContext context) noexcept;

public: // implement Diagnostic
    [[nodiscard]] auto sourcePath() const noexcept -> text::String override;
    [[nodiscard]] auto toString() const noexcept -> text::String override;
    [[nodiscard]] auto toTextDocument(const i18n::DisplayTextMapConstPtr &displayText) const
        -> text::TextDocument override;

private:
    /// Append a path value to diagnostic document content.
    static void appendPath(text::TextNode &content, const text::String &path);

private:
    PathErrorContext _context;
};

}
