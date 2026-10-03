// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Diagnostic.hpp"
#include "Document_fwd.hpp"
#include "Statement.hpp"
#include "Token.hpp"

#include "impl/Reader_fwd.hpp"

#include "../SourceIdentifier_fwd.hpp"

#include "../../util/List.hpp"

namespace erbsland::conf::syntax {

/// An immutable, lossless syntax snapshot with locally resolved statement paths.
/// @tested{SyntaxParserTest SyntaxFormatterTest}
class Document final {
    friend class impl::Reader;

public: // accessors
    /// Get the unmodified original source.
    [[nodiscard]] auto text() const noexcept -> const text::String & { return _text; }
    /// Get all source tokens in order, excluding an end sentinel.
    [[nodiscard]] auto tokens() const noexcept -> const util::List<TokenPtr> & { return _tokens; }
    /// Get all recognized statements in source order.
    [[nodiscard]] auto statements() const noexcept -> const util::List<Statement> & { return _statements; }
    /// Get source diagnostics.
    [[nodiscard]] auto diagnostics() const noexcept -> const util::List<Diagnostic> & { return _diagnostics; }
    /// Get source line ranges, including their newline bytes.
    [[nodiscard]] auto lines() const noexcept -> const util::List<unit::ByteRange> & { return _lines; }
    /// Get the source identifier.
    [[nodiscard]] auto sourceIdentifier() const noexcept -> const SourceIdentifierPtr & { return _identifier; }
    /// Test whether any input errors were detected.
    [[nodiscard]] auto hasErrors() const noexcept -> bool { return !_diagnostics.isEmpty(); }
    /// Find the tokens intersecting a source line.
    /// @param line Zero-based line index.
    /// @return A stored list, or a shared empty list when the line does not exist.
    [[nodiscard]] auto tokensForLine(unit::LineIndex line) const noexcept -> const util::List<TokenPtr> &;

private:
    text::String _text;                           ///< Owned source snapshot.
    SourceIdentifierPtr _identifier;              ///< Source identity.
    util::List<TokenPtr> _tokens;                 ///< Lossless tokens.
    util::List<Statement> _statements;            ///< Recognized statements.
    util::List<Diagnostic> _diagnostics;          ///< Input errors.
    util::List<unit::ByteRange> _lines;           ///< Physical lines.
    util::List<util::List<TokenPtr>> _lineTokens; ///< Tokens intersecting each physical line.
};

}
