// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../BlockString.hpp"

#include <utility>
#include <vector>

namespace erbsland::cterm::impl {

/// Helper that keeps string wrapping and line splitting logic out of the `BlockStringEditor` API type.
class BlockStringWrapper final {
public:
    /// Create a wrapper tool for one terminal string.
    /// @param str The read-only string to process.
    explicit BlockStringWrapper(const BlockString &str) noexcept : _str{str} {}

    // defaults/deletions
    ~BlockStringWrapper() = default;
    BlockStringWrapper(const BlockStringWrapper &) = delete;
    BlockStringWrapper(BlockStringWrapper &&) = delete;
    auto operator=(const BlockStringWrapper &) -> BlockStringWrapper & = delete;
    auto operator=(BlockStringWrapper &&) -> BlockStringWrapper & = delete;

public:
    /// Wrap the referenced string into terminal-width limited lines.
    /// @param width The maximum terminal width in cells. Must be greater than zero.
    /// @param paragraphSpacing The spacing to use between newline-separated paragraphs.
    /// @return A sequence of wrapped lines.
    [[nodiscard]] auto wrapIntoLines(int width, ParagraphSpacing paragraphSpacing) const noexcept
        -> std::vector<BlockStringEditor>;
    /// Split the referenced string at newline characters.
    /// @return A sequence of lines without the newline characters.
    [[nodiscard]] auto splitLines() const noexcept -> std::vector<BlockStringEditor>;

private:
    /// Wrap this paragraph into display-width-limited lines.
    [[nodiscard]] auto wrapParagraphIntoLines(int width) const noexcept -> std::vector<BlockStringEditor>;
    /// Reset the pending spacing token that is only emitted when followed by another word on the same line.
    static void clearPendingSpacing(BlockStringEditor &pendingSpacing, int &pendingSpacingWidth) noexcept;
    /// Finish the current wrapped line and append it to the result.
    static void finishWrappedLine(
        BlockStringEditor &line,
        int &lineWidth,
        BlockStringEditor &pendingSpacing,
        int &pendingSpacingWidth,
        std::vector<BlockStringEditor> &lines) noexcept;
    /// Store a spacing token until the next word is known to fit on the current line.
    static void appendSpacingToken(
        BlockStringEditor &spacing,
        int spacingWidth,
        const BlockStringEditor &line,
        BlockStringEditor &pendingSpacing,
        int &pendingSpacingWidth) noexcept;
    /// Append one word token to the current wrapped output.
    static void appendWordToken(
        BlockStringEditor &word,
        int wordWidth,
        int width,
        BlockStringEditor &line,
        int &lineWidth,
        BlockStringEditor &pendingSpacing,
        int &pendingSpacingWidth,
        std::vector<BlockStringEditor> &lines) noexcept;
    /// Start a new wrapped line with the given word or split it into standalone lines if it is too wide.
    static void startWrappedLine(
        BlockStringEditor &word,
        int wordWidth,
        int width,
        BlockStringEditor &line,
        int &lineWidth,
        std::vector<BlockStringEditor> &lines) noexcept;
    /// Split one oversized word into separate wrapped lines.
    static void splitWordIntoWrappedLines(
        const BlockStringEditor &word, int wordWidth, int width, std::vector<BlockStringEditor> &lines) noexcept;
    /// Find the exclusive end index for the next part of an oversized word.
    [[nodiscard]] static auto findWrappedWordSplitIndex(
        const BlockString &word, BlockIndex startIndex, int width) noexcept -> BlockIndex;

private:
    const BlockString &_str;
};

}
