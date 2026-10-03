// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/StringSplitter.hpp"
#include "../../Source.hpp"
#include "../../SourceIdentifier.hpp"

namespace erbsland::conf::syntax::impl {

/// A snapshot source retaining all lines, including lines rejected by strict source limits.
/// @tested{SyntaxParserTest}
class SnapshotSource final : public Source {
public:
    /// Create a snapshot source.
    /// @param text Owned source text.
    /// @param identifier Source identity.
    SnapshotSource(text::String text, SourceIdentifierPtr identifier) :
        _lines{std::move(text), text::Char{U'\n'}, text::StringSplitMode::KeepSeparator},
        _identifier{std::move(identifier)} {}

public: // implement Source
    auto identifier() const noexcept -> SourceIdentifierPtr override { return _identifier; }
    void open() override { _open = true; }
    auto isOpen() const noexcept -> bool override { return _open; }
    auto atEnd() const noexcept -> bool override { return _lines.isAtEnd(); }
    auto readLine() -> text::String override { return _lines.next(); }
    auto codeSnippet(unit::CodeLocation) noexcept -> std::optional<text::CodeSnippet> override { return {}; }
    void close() noexcept override { _open = false; }

private:
    text::StringSplitter _lines;     ///< Owning line splitter.
    SourceIdentifierPtr _identifier; ///< Source identifier.
    bool _open{};                    ///< Open state.
};

}
