// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../bgeo/BlockMargins.hpp"

#include <algorithm>

namespace erbsland::cterm {

/// Shared indentation and margin settings for paragraph-like text rendering.
class ParagraphIndents final {
public:
    /// Special value that makes `firstLineIndent()` or `wrappedLineIndent()` reuse `lineIndent()`.
    static constexpr auto cUseLineIndent = -1;

public:
    /// Create default indents without margins.
    constexpr ParagraphIndents() noexcept = default;
    /// Create indents with the same value for all paragraph lines.
    /// @param lineIndent The indent to use for all lines.
    explicit constexpr ParagraphIndents(const int lineIndent) noexcept : _lineIndent{std::max(lineIndent, 0)} {}
    /// Create indents and margins with explicit values.
    /// @param lineIndent The indent for all lines.
    /// @param firstLineIndent The indent for the first line, or `cUseLineIndent`.
    /// @param wrappedLineIndent The indent for wrapped lines, or `cUseLineIndent`.
    /// @param margins The margins around the rendered paragraph area.
    constexpr ParagraphIndents(
        const int lineIndent,
        const int firstLineIndent,
        const int wrappedLineIndent,
        const bgeo::BlockMargins margins) noexcept :
        _lineIndent{std::max(lineIndent, 0)},
        _firstLineIndent{std::max(firstLineIndent, cUseLineIndent)},
        _wrappedLineIndent{std::max(wrappedLineIndent, cUseLineIndent)},
        _margins{margins} {}

public:
    auto operator==(const ParagraphIndents &) const noexcept -> bool = default;
    auto operator!=(const ParagraphIndents &) const noexcept -> bool = default;

public:
    /// Get the indent for all lines.
    [[nodiscard]] constexpr auto lineIndent() const noexcept -> int { return _lineIndent; }
    /// Set the indent for all lines.
    /// @param indent The new indent value. `>=0`
    constexpr void setLineIndent(const int indent) noexcept { _lineIndent = std::max(indent, 0); }
    /// Get the indent for the first line.
    /// @return The resolved first-line indent.
    [[nodiscard]] constexpr auto firstLineIndent() const noexcept -> int {
        return _firstLineIndent == cUseLineIndent ? _lineIndent : _firstLineIndent;
    }
    /// Set the indent for the first line.
    /// @param indent The new indent value. `>=0` or `cUseLineIndent` to use `lineIndent()`.
    constexpr void setFirstLineIndent(const int indent) noexcept {
        _firstLineIndent = std::max(indent, cUseLineIndent);
    }
    /// Get the indent for wrapped lines.
    /// @return The resolved wrapped-line indent.
    [[nodiscard]] constexpr auto wrappedLineIndent() const noexcept -> int {
        return _wrappedLineIndent == cUseLineIndent ? _lineIndent : _wrappedLineIndent;
    }
    /// Set the indent for wrapped lines.
    /// @param indent The new indent value. `>=0` or `cUseLineIndent` to use `lineIndent()`.
    constexpr void setWrappedLineIndent(const int indent) noexcept {
        _wrappedLineIndent = std::max(indent, cUseLineIndent);
    }
    /// Get the margins around the paragraph.
    [[nodiscard]] constexpr auto margins() const noexcept -> const bgeo::BlockMargins & { return _margins; }
    /// Set the margins around the paragraph.
    /// @param margins The new margins.
    constexpr void setMargins(const bgeo::BlockMargins margins) noexcept { _margins = margins; }

private:
    int _lineIndent{0};                     ///< Indent for all lines.
    int _firstLineIndent{cUseLineIndent};   ///< Indent for the first line, or `cUseLineIndent`.
    int _wrappedLineIndent{cUseLineIndent}; ///< Indent for wrapped lines, or `cUseLineIndent`.
    bgeo::BlockMargins _margins{0};         ///< bgeo::BlockMargins around the paragraph area.
};

}
