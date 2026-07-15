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
    explicit PathErrorDiagnostic(PathErrorContext context) noexcept;

public: // implement Diagnostic
    [[nodiscard]] auto sourcePath() const noexcept -> text::StringView override;
    [[nodiscard]] auto toString() const noexcept -> text::StringView override;
    [[nodiscard]] auto toTextDocument(const i18n::DisplayTextMapConstPtr &displayText) const
        -> text::TextDocument override;

private:
    static void appendPath(text::TextNode &content, text::StringView path);

private:
    PathErrorContext _context;
};

}
