// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LayoutLine.hpp"

#include "../Formatter.hpp"
#include "../TokenKind.hpp"

#include "../../../text/StringEditor.hpp"
#include "../../../text/StringList.hpp"

namespace erbsland::conf::syntax::impl {

/// Source-preserving layout engine operating on recognized statements.
/// @tested{SyntaxFormatterTest}
class Writer final {
public:
    // defaults
    Writer() = default;

public: // formatting
    /// Format a complete snapshot.
    /// @param document Source snapshot.
    /// @param format Cascade configuration.
    /// @return Formatted text and source diagnostics.
    auto write(const DocumentPtr &document, const Format &format) -> FormatResult;

private:
    /// Render one section declaration.
    auto section(const Statement &statement) -> text::String;
    /// Render one complete assignment.
    auto assignment(const Statement &statement) -> text::String;
    /// Render structural tokens while preserving literal payload.
    auto renderLine(
        const LayoutLine &line, bool first, const util::List<unit::CpCount> &columns = {}, bool preserveSpacing = false)
        -> text::String;
    /// Reposition a non-multiline value without crossing comments or payload lines.
    auto reflowValue() -> std::optional<text::String>;
    /// Apply the placement policy to an indivisible value and its trailing comment.
    auto placeValue(
        const text::String &prefix, const text::String &value, bool wasNextLine, const text::String &finalNewline) const
        -> text::String;
    /// Test whether the complete value belongs on a continuation line.
    auto valueOnNextLine(unit::CpCount sameLineWidth, unit::CpCount nextLineWidth, bool wasNextLine) const -> bool;
    /// Reflow an eligible vector without detaching comments.
    auto reflowVector(const Statement &statement) -> std::optional<text::String>;
    /// Collect an eligible vector and its assignment prefix without detaching comments.
    auto vectorValues(LayoutLine &prefix) const -> text::StringList;
    /// Render an eligible compact vector when the placement policy fits it within the width.
    auto compactVector(const text::String &prefix, const text::StringList &values) const -> std::optional<text::String>;
    /// Join vector values as indented bullet rows.
    auto multilineVector(const text::String &prefix, const text::StringList &values) const -> text::String;
    /// Identify multiline payload and opening-delimiter tokens.
    static auto isMultilineToken(TokenKind kind) noexcept -> bool;
    /// Compute column widths when alignment fits.
    auto matrixColumns() -> util::List<unit::CpCount>;
    /// Apply opt-in normalization to a source token.
    auto tokenText(const Token &token) const -> text::String;
    /// Normalize identifier case and word separators.
    auto nameText(const text::String &text, WordCase wordCase, WordSeparator separator) const -> text::String;
    /// Create the selected continuation indentation.
    auto indentation() const -> text::String;
    /// Choose the newline convention for newly inserted lines.
    auto newline() const -> text::String;
    /// Group a statement into physical source lines.
    void splitLines(const Statement &statement);
    /// Insert spacing according to a count or target column.
    static void pad(text::StringEditor &out, PositionMode mode, unit::CpCount count, unit::CpCount minimum = {});
    /// Reject invalid style properties before rendering.
    static void validate(const FormatStyle &style);

private:
    DocumentPtr _document;         ///< Shared input during formatting.
    FormatStyle _style;            ///< Resolved current style.
    util::List<LayoutLine> _lines; ///< Current physical statement lines.
};

}
