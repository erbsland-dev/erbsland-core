// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Diagnostic.hpp"

#include "../../i18n/DisplayTextMap_fwd.hpp"
#include "../../text/String.hpp"
#include "../../util/List.hpp"

namespace erbsland::err::impl {

/// Default diagnostic for a plain exception.
/// @tested{DiagnosticTest}
class ExceptionDiagnostic final : public Diagnostic {
    struct Field final {
        text::String label;
        text::String value;
        text::String style;
    };

public:
    /// Create a diagnostic from a message.
    /// @param message The diagnostic message.
    /// @param displayText The display text map to use, or nullptr for the default texts.
    explicit ExceptionDiagnostic(text::String message, const i18n::DisplayTextMapConstPtr &displayText = {}) noexcept;

public:
    /// Set a source name.
    auto setSourceName(text::String sourceName) noexcept -> ExceptionDiagnostic &;
    /// Set a source path.
    auto setSourcePath(text::String sourcePath) noexcept -> ExceptionDiagnostic &;
    /// Set a source location.
    auto setLocation(unit::CodeLocation location) noexcept -> ExceptionDiagnostic &;
    /// Append a labeled detail field.
    auto appendField(text::String label, text::String value, text::String style = {}) -> ExceptionDiagnostic &;

public: // implement Diagnostic
    [[nodiscard]] auto sourceName() const noexcept -> text::String override;
    [[nodiscard]] auto sourcePath() const noexcept -> text::String override;
    [[nodiscard]] auto location() const noexcept -> unit::CodeLocation override;
    [[nodiscard]] auto toString() const noexcept -> text::String override;
    [[nodiscard]] auto toTextDocument(const i18n::DisplayTextMapConstPtr &displayText) const
        -> text::TextDocument override;

private:
    text::String _message;                     ///< Diagnostic message.
    text::String _sourceName;                  ///< Optional source name.
    text::String _sourcePath;                  ///< Optional source path.
    unit::CodeLocation _location;              ///< Optional source location.
    util::List<Field> _fields;                 ///< Optional detail fields.
    i18n::DisplayTextMapConstPtr _displayText; ///< Captured display texts, if supplied.
};

}
