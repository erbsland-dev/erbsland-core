// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SourceBlock.hpp"

#include <erbsland/ByteRange.hpp>
#include <erbsland/cpp/CppToken.hpp>
#include <erbsland/ItemIndex.hpp>
#include <erbsland/LineCount.hpp>
#include <erbsland/LineIndex.hpp>
#include <erbsland/List.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringEditor.hpp>

#include <utility>

namespace tool::anti_patterns {

/// A single-tokenization view of source text shared by all detectors for one file.
class SourceIndex final {
public:
    /// Tokenize one source snapshot and build source-coordinate and scope indexes.
    SourceIndex(el::Path relativePath, el::String text);

public: // accessors
    /// Get the project-relative source path.
    [[nodiscard]] auto path() const noexcept -> const el::Path & { return _path; }
    /// Get the original source snapshot.
    [[nodiscard]] auto text() const noexcept -> const el::String & { return _text; }
    /// Get source text with comments and literals replaced by spaces.
    [[nodiscard]] auto masked() const noexcept -> const el::String & { return _masked; }
    /// Get the one-pass lexical tokens.
    [[nodiscard]] auto tokens() const noexcept -> const el::List<el::cpp::CppToken> & { return _tokens; }
    /// Get indexes of significant tokens.
    [[nodiscard]] auto codeTokens() const noexcept -> const el::List<el::ItemIndex> & { return _code; }
    /// Get indexes of comment tokens.
    [[nodiscard]] auto comments() const noexcept -> const el::List<el::ItemIndex> & { return _comments; }
    /// Get indexes of literal tokens.
    [[nodiscard]] auto literals() const noexcept -> const el::List<el::ItemIndex> & { return _literals; }
    /// Get indexed brace scopes.
    [[nodiscard]] auto blocks() const noexcept -> const el::List<SourceBlock> & { return _blocks; }
    /// Get the number of physical source lines.
    [[nodiscard]] auto lineCount() const noexcept -> el::LineCount;
    /// Detect the generated-source marker near the start.
    [[nodiscard]] auto isGenerated() const noexcept -> bool { return _generated; }

public: // source queries
    /// Map a byte index to a zero-based physical line index.
    /// @param index Source byte position; EOF after a newline maps to the trailing empty line.
    /// @return The line containing the position.
    [[nodiscard]] auto lineIndex(el::ByteIndex index) const -> el::LineIndex;
    /// Get the first byte position of a line.
    /// @param index Zero-based physical line index.
    /// @return The line start, or EOF when the index is beyond the stored lines.
    [[nodiscard]] auto lineStart(el::LineIndex index) const -> el::ByteIndex;
    /// Get the exclusive byte position of a line, excluding its LF terminator.
    /// @param index Zero-based physical line index.
    /// @return The LF position, or EOF for a final unterminated or out-of-bounds line.
    [[nodiscard]] auto lineEnd(el::LineIndex index) const -> el::ByteIndex;
    /// Get one original physical line.
    /// @param index Zero-based physical line index.
    /// @return The line without its LF terminator, retaining any CR.
    [[nodiscard]] auto line(el::LineIndex index) const -> el::String;
    /// Get a source span, optionally masking non-code.
    /// @param range Source byte range, clamped to the source length.
    /// @param masked Whether to return the corresponding masked text.
    /// @return The selected source text.
    [[nodiscard]] auto slice(el::ByteRange range, bool masked = false) const -> el::String;
    /// Find the end of the current declaration or statement.
    [[nodiscard]] auto statementEnd(el::ByteIndex start) const -> el::ByteIndex;
    /// Detect a delimiter opened before a location.
    [[nodiscard]] auto hasUnclosedDelimiterBefore(el::ByteIndex offset, el::String opening, el::String closing) const
        -> bool;
    /// Test whether a location is at namespace scope.
    [[nodiscard]] auto namespaceScope(el::ByteIndex offset) const -> bool;
    /// Test whether a location belongs to an ERBSLAND_OS branch.
    [[nodiscard]] auto insideOsConditional(el::ByteIndex offset) const -> bool;
    /// Find the innermost brace scope around a location.
    /// @param offset Source byte position; opening and closing braces are excluded.
    /// @return The block index, or ItemIndex::noIndex() outside all scopes.
    [[nodiscard]] auto enclosingBlock(el::ByteIndex offset) const -> el::ItemIndex;
    /// Get the parent brace scope when present.
    [[nodiscard]] auto parent(const SourceBlock &block) const -> const SourceBlock *;
    /// Find the start of a scoped type declaration.
    [[nodiscard]] auto typeStart(const SourceBlock &block) const -> el::ByteIndex;
    /// Read an adjacent inline acceptance comment for a rule.
    [[nodiscard]] auto inlineReason(const el::String &rule, el::ByteRange range) const -> el::String;
    /// Render a compact source excerpt for the report.
    [[nodiscard]] auto snippet(el::ByteRange range) const -> el::String;

private:
    /// Index physical source lines once.
    void buildLines();
    /// Tokenize once and record masked source and token indexes.
    void buildTokens();
    /// Build balanced scope and declaration indexes.
    void buildScopes();
    /// Classify one brace scope from its leading tokens.
    void classify(SourceBlock &block);
    /// Record conditional preprocessing state.
    void recordDirective(const el::cpp::CppToken &token, el::List<std::pair<el::ByteIndex, bool>> &stack);
    /// Append spaces preserving the byte length of non-code text.
    static void appendMask(el::StringEditor &output, const el::String &text);

private:
    el::Path _path;                      ///< Relative source path.
    el::String _text;                    ///< Immutable source snapshot.
    el::String _masked;                  ///< Same-length source with non-code tokens blanked.
    el::List<el::ByteIndex> _lines;      ///< Physical line starts.
    el::List<el::cpp::CppToken> _tokens; ///< Significant and trivia tokens in source order.
    el::List<el::ItemIndex> _code;       ///< Indexes of non-trivia tokens.
    el::List<el::ItemIndex> _comments;   ///< Indexes into token storage.
    el::List<el::ItemIndex> _literals;   ///< Indexes into token storage.
    el::List<SourceBlock> _blocks;       ///< Brace scopes in opening order.
    el::List<el::ByteRange> _osRanges;   ///< Platform conditional ranges.
    bool _generated = false;             ///< Generated marker in first ten lines.
};

}
