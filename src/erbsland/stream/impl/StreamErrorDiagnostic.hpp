// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../StreamErrorContext.hpp"

#include "../../err/Diagnostic.hpp"
#include "../../text/TextNode_fwd.hpp"

namespace erbsland::stream::impl {

/// Diagnostic document for a stream error.
/// @tested{DiagnosticTest}
class StreamErrorDiagnostic final : public err::Diagnostic {
public:
    /// Create a diagnostic from stream error context.
    /// @param context The context to render.
    explicit StreamErrorDiagnostic(StreamErrorContext context) noexcept;

public: // implement Diagnostic
    [[nodiscard]] auto sourcePath() const noexcept -> text::String override;
    [[nodiscard]] auto toString() const noexcept -> text::String override;
    [[nodiscard]] auto toTextDocument(const i18n::DisplayTextMapConstPtr &displayText) const
        -> text::TextDocument override;

private:
    /// Append a path value to diagnostic document content.
    static void appendPath(text::TextNode &content, const text::String &path);

private:
    StreamErrorContext _context; ///< Stream error context rendered by this diagnostic.
};

}
