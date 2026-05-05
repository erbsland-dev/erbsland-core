// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../BlockStringView.hpp"

#include <utility>
#include <vector>

namespace erbsland::cterm::impl {

/// Helper that keeps string wrapping and line splitting logic out of the `BlockString` API type.
class BlockStringWrapper final {
public:
    /// Create a wrapper tool for one terminal string.
    /// @param str The string view to process.
    explicit BlockStringWrapper(const BlockStringView &str) noexcept : _str{str} {}

    // default/delete
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
        -> std::vector<BlockString>;
    /// Split the referenced string at newline characters.
    /// @return A sequence of lines without the newline characters.
    [[nodiscard]] auto splitLines() const noexcept -> std::vector<BlockString>;

private:
    [[nodiscard]] auto wrapParagraphIntoLines(int width) const noexcept -> std::vector<BlockString>;
    /// Reset the pending spacing token that is only emitted when followed by another word on the same line.
    static void clearPendingSpacing(BlockString &pendingSpacing, int &pendingSpacingWidth) noexcept;
    /// Finish the current wrapped line and append it to the result.
    static void finishWrappedLine(
        BlockString &line,
        int &lineWidth,
        BlockString &pendingSpacing,
        int &pendingSpacingWidth,
        std::vector<BlockString> &lines) noexcept;
    /// Store a spacing token until the next word is known to fit on the current line.
    static void appendSpacingToken(
        BlockString &spacing,
        int spacingWidth,
        const BlockString &line,
        BlockString &pendingSpacing,
        int &pendingSpacingWidth) noexcept;
    /// Append one word token to the current wrapped output.
    static void appendWordToken(
        BlockString &word,
        int wordWidth,
        int width,
        BlockString &line,
        int &lineWidth,
        BlockString &pendingSpacing,
        int &pendingSpacingWidth,
        std::vector<BlockString> &lines) noexcept;
    /// Start a new wrapped line with the given word or split it into standalone lines if it is too wide.
    static void startWrappedLine(
        BlockString &word,
        int wordWidth,
        int width,
        BlockString &line,
        int &lineWidth,
        std::vector<BlockString> &lines) noexcept;
    /// Split one oversized word into separate wrapped lines.
    static void splitWordIntoWrappedLines(
        const BlockString &word, int wordWidth, int width, std::vector<BlockString> &lines) noexcept;
    /// Find the exclusive end index for the next part of an oversized word.
    [[nodiscard]] static auto findWrappedWordSplitIndex(
        const BlockStringView &word, BlockIndex startIndex, int width) noexcept -> BlockIndex;

private:
    const BlockStringView &_str;
};

}
