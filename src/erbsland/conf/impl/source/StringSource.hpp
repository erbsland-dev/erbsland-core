// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/StringSplitter.hpp"
#include "../../Source.hpp"

namespace erbsland::conf::impl {

/// A configuration source backed by an owning Core string.
///
/// Lines are captured as COW slices of the original string. Tolerant character scanning is intentional: malformed
/// UTF-8 must remain part of the returned line so `CharStream` can report it at the precise document position.
/// @tested{StringSourceTest}
class StringSource final : public Source {
public:
    /// Create a new string source.
    explicit StringSource(text::String text) noexcept;

    // defaults
    ~StringSource() override = default;

public:
    [[nodiscard]] auto identifier() const noexcept -> SourceIdentifierPtr override;
    void open() override;
    [[nodiscard]] auto isOpen() const noexcept -> bool override { return _isOpen; }
    [[nodiscard]] auto atEnd() const noexcept -> bool override { return _atEnd; }
    [[nodiscard]] auto readLine() -> text::String override;
    [[nodiscard]] auto codeSnippet(unit::CodeLocation location) noexcept -> std::optional<text::CodeSnippet> override;
    void close() noexcept override { _isOpen = false; }

private:
    /// Throw an error for a source line exceeding the configured maximum length.
    [[noreturn]] void throwLineLengthExceeded();

private:
    text::StringSplitter _lineSplitter; ///< The zero-copy line splitter for the source text.
    bool _isOpen{false};                ///< If this source is open.
    bool _atEnd{false};                 ///< If the end of the source was reached.
};

}
