// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Reader_fwd.hpp"

#include "../Document.hpp"

#include "../../ConfErrorContext.hpp"
#include "../../impl/decoder/TokenDecoder_fwd.hpp"
#include "../../impl/lexer/LexerToken.hpp"

namespace erbsland::conf::syntax::impl {

/// Recovering driver over the shared ELCL recognition routines.
/// @tested{SyntaxParserTest}
class Reader final {
public:
    /// Read a complete source snapshot.
    /// @param text Source text.
    /// @param identifier Source identifier.
    /// @return Lossless syntax document.
    auto read(text::String text, SourceIdentifierPtr identifier) -> DocumentPtr;

private:
    /// Initialize the source snapshot and its persistent syntax decoder.
    void initialize(text::String text, SourceIdentifierPtr identifier);
    /// Read and finalize one top-level source part.
    void readPart();
    /// Build line token lists and retain source line-limit diagnostics.
    void indexLines();
    /// Retain a recognized token with its exact source range.
    void append(const conf::impl::LexerToken &token);
    /// Record a source diagnostic and invalidate the current statement.
    void diagnose(const ConfErrorContext &context);
    /// Resolve and commit the current statement.
    void finishStatement();
    /// Resolve local section and assignment name paths.
    void resolve(Statement &statement);

private:
    DocumentPtr _document;                ///< Result under construction.
    conf::impl::TokenDecoderPtr _decoder; ///< Shared token decoder.
    std::optional<Statement> _statement;  ///< Current statement.
    unit::CodeLocation _position{unit::LineIndex{}, unit::ColumnIndex{}, unit::CpIndex{}}; ///< Next source location.
    unit::ByteIndex _offset{};                                                             ///< Exact next source byte.
    NamePath _absolute;               ///< Last known absolute declaration.
    std::optional<NamePath> _section; ///< Current resolved section.
};

}
