// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "RegExErrorContext.hpp"

#include "../err/RuntimeError.hpp"

namespace erbsland::re {

/// An error raised by regular-expression parsing, compilation, diagnostics, or matching.
/// @seedoc{/reference/re/regular_expressions}
/// @tested{RegExErrorTest}
class RegExError final : public err::RuntimeError {
public:
    /// Create an error with a concise title.
    RegExError(ErrorCategory category, text::String title) noexcept;
    /// Create an error with a title and source location.
    RegExError(ErrorCategory category, text::String title, unit::CodeLocation location) noexcept;
    /// Create an error with a title, description, and optional source location.
    RegExError(
        ErrorCategory category,
        text::String title,
        text::String description,
        unit::CodeLocation location = {}) noexcept;
    /// Create an error from complete context.
    explicit RegExError(RegExErrorContext context) noexcept;

    // defaults
    ~RegExError() override = default;
    RegExError(const RegExError &) = default;
    RegExError(RegExError &&) = default;
    auto operator=(const RegExError &) -> RegExError & = default;
    auto operator=(RegExError &&) -> RegExError & = default;

public: // implement RuntimeError
    /// Create a compact one-line summary containing category, title and description.
    [[nodiscard]] auto toString() const noexcept -> text::String override;
    /// Create the complete structured diagnostic.
    [[nodiscard]] auto diagnostic() const -> err::DiagnosticConstPtr override;

public: // accessors
    /// Get the complete error context.
    [[nodiscard]] auto context() const noexcept -> const RegExErrorContext & { return _context; }
    /// Get the error category.
    [[nodiscard]] auto category() const noexcept -> ErrorCategory { return _context.category(); }
    /// Get the concise error title.
    [[nodiscard]] auto title() const noexcept -> const text::String & { return _context.title(); }
    /// Get the detailed error description.
    [[nodiscard]] auto description() const noexcept -> const text::String & { return _context.description(); }
    /// Get the source location.
    [[nodiscard]] auto location() const noexcept -> unit::CodeLocation { return _context.location(); }
    /// Get the source line index.
    [[nodiscard]] auto line() const noexcept -> unit::LineIndex { return _context.line(); }
    /// Get the source column index.
    [[nodiscard]] auto column() const noexcept -> unit::ColumnIndex { return _context.column(); }
    /// Get the source code-point position.
    [[nodiscard]] auto position() const noexcept -> unit::CpIndex { return _context.position(); }

public:
    /// Create a copy of this error with the given zero-based line index.
    [[nodiscard]] auto withLineNumber(unit::LineIndex lineNumber) const noexcept -> RegExError;

private:
    RegExErrorContext _context;
};

}
