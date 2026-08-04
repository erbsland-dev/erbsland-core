// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LayoutLine.hpp"
#include "LayoutLineToken.hpp"

#include "../../BlockRange.hpp"

#include <algorithm>
#include <vector>

namespace erbsland::cterm::impl::paragraph {

/// A pre-tokenized source line ready for physical line layout.
class LayoutPreparedSourceLine final {
public:
    /// Create a prepared source line for the given source range.
    /// @param sourceRange The original source-line range in the input text.
    explicit LayoutPreparedSourceLine(BlockRange sourceRange) : sourceLine{sourceRange} {}

    // defaults
    ~LayoutPreparedSourceLine() = default;
    LayoutPreparedSourceLine(const LayoutPreparedSourceLine &) = delete;
    LayoutPreparedSourceLine(LayoutPreparedSourceLine &&) noexcept = default;
    auto operator=(const LayoutPreparedSourceLine &) -> LayoutPreparedSourceLine & = delete;
    auto operator=(LayoutPreparedSourceLine &&) noexcept -> LayoutPreparedSourceLine & = default;

public:
    /// Test whether this prepared source line contains any tokens.
    /// @return `true` if this prepared source line contains no tokens.
    [[nodiscard]] auto empty() const noexcept -> bool { return tokens.empty(); }
    /// Append one word token and flush pending spacing tokens in front of it.
    /// @param startIndex The first source character index of the word.
    /// @param length The number of source characters in the word.
    /// @param displayWidth The cached display width of the word.
    void appendWord(BlockIndex startIndex, BlockCount length, int displayWidth) noexcept {
        if (length.isZero()) {
            return;
        }
        flushPendingTokens();
        tokens.emplace_back(LayoutLineToken::Type::Word, startIndex, length, displayWidth);
        _hasWordToken = true;
    }
    /// Append one source range that must not be split across physical lines.
    /// @param startIndex The first source character index.
    /// @param length The number of source characters.
    /// @param displayWidth The cached display width.
    void appendIndivisibleWord(BlockIndex startIndex, BlockCount length, int displayWidth) noexcept {
        if (length.isZero()) {
            return;
        }
        flushPendingTokens();
        tokens.emplace_back(LayoutLineToken::Type::IndivisibleWord, startIndex, length, displayWidth);
        _hasWordToken = true;
    }
    /// Append one pending tab token that may be flushed later.
    /// @param index The source character index of the tab.
    void appendPendingTab(BlockIndex index) noexcept {
        _pendingTokens.emplace_back(LayoutLineToken::Type::Tab, index, BlockCount::one(), 0);
    }
    /// Append one pending separator-space token, merging adjacent pending separator runs.
    /// @param index The source character index of the separator.
    void appendPendingSeparatorSpace(BlockIndex index, int displayWidth = 1) noexcept {
        if (!_pendingTokens.empty() && _pendingTokens.back().type() == LayoutLineToken::Type::SeparatorSpace) {
            const auto &separator = _pendingTokens.back();
            _pendingTokens.back() = LayoutLineToken{
                LayoutLineToken::Type::SeparatorSpace,
                separator.startIndex(),
                separator.length() + BlockCount::one(),
                std::max(separator.displayWidth(), displayWidth)};
            return;
        }
        _pendingTokens.emplace_back(LayoutLineToken::Type::SeparatorSpace, index, BlockCount::one(), displayWidth);
    }
    /// Flush the remaining pending spacing tokens into the prepared token list.
    void finish() noexcept {
        flushPendingTokens();
        _pendingTokens.clear();
    }

public:
    BlockRange sourceLine;               ///< The original source line range.
    std::vector<LayoutLineToken> tokens; ///< The extracted tokens of the source line.

private:
    /// Append pending spacing tokens after the line has gained a word token.
    void flushPendingTokens() noexcept {
        if (!_hasWordToken) {
            for (const auto &token : _pendingTokens) {
                if (token.type() == LayoutLineToken::Type::Tab) {
                    tokens.push_back(token);
                }
            }
            _pendingTokens.clear();
            return;
        }
        for (const auto &token : _pendingTokens) {
            tokens.push_back(token);
        }
        _pendingTokens.clear();
    }

private:
    std::vector<LayoutLineToken> _pendingTokens; ///< Spacing tokens waiting for the next emitted word.
    bool _hasWordToken{false};                   ///< Tracks whether a word token was already emitted.
};

}
