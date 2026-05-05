// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockStringData.hpp"

#include "../BlockString.hpp"
#include "../BlockStringView.hpp"

#include "../../text/EncodingErrorMode.hpp"
#include "../../text/StringView.hpp"
#include "../../text/u32/U32StringView.hpp"

#include <utility>

namespace erbsland::cterm::impl {

/// Unique mutable builder optimized for repeated terminal-string assembly.
class BlockStringBuilder {
public:
    /// Create an empty string builder.
    BlockStringBuilder() = default;

    // defaults
    ~BlockStringBuilder() = default;
    BlockStringBuilder(const BlockStringBuilder &) = delete;
    BlockStringBuilder(BlockStringBuilder &&) noexcept = default;
    auto operator=(const BlockStringBuilder &) -> BlockStringBuilder & = delete;
    auto operator=(BlockStringBuilder &&) noexcept -> BlockStringBuilder & = default;

public:
    /// Reserve storage for at least the given number of characters.
    /// @param count The requested capacity.
    void reserve(const BlockCount count) { _data.reserve(count.toSizeT()); }
    /// Remove all characters while keeping the allocated capacity for reuse.
    void clear() noexcept { _data.clear(); }
    /// Test if the builder currently holds no characters.
    /// @return `true` if the builder is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _data.size() == 0; }
    /// Get the number of stored characters.
    /// @return The current character count.
    [[nodiscard]] auto length() const noexcept -> BlockCount { return BlockCount::fromSizeT(_data.size()); }
    /// Get the cached display width of the built text.
    /// @return The display width in terminal cells.
    [[nodiscard]] auto displayWidth() const noexcept -> int { return _data.displayWidth(); }
    /// Append one terminal character as-is.
    /// @param character The character to append.
    void append(const Block &character) noexcept { _data.append(character); }
    /// Append one styled terminal-string view as-is.
    /// @param view The string view to append.
    void append(const BlockStringView &view) noexcept {
        if (view.isEmpty()) {
            return;
        }
        auto &chars = _data.chars();
        chars.insert(chars.end(), view.begin(), view.end());
        _data.addDisplayWidth(view.displayWidth());
    }
    /// Append one terminal-string view with a resolved base style.
    /// @param view The string view to append.
    /// @param style The base style used for inherited color components or attributes.
    void appendWithBaseStyle(const BlockStringView &view, const BlockStyle style) noexcept {
        if (view.isEmpty()) {
            return;
        }
        for (const auto &character : view) {
            const auto resolved = character.withBase(style);
            _data.append(resolved);
        }
    }
    /// Append UTF-32 text using one uniform style.
    /// @param text The UTF-32 text to append.
    /// @param style The style applied to the appended characters.
    void appendStyled(const text::U32StringView &text, const BlockStyle style) noexcept {
        _data.appendCharacters(text, style.color(), style.attributes());
    }
    /// Append UTF-8 text using one uniform style.
    /// @param text The UTF-8 text to append.
    /// @param style The style applied to the appended characters.
    /// @param encodingErrors How malformed UTF-8 is handled.
    void appendStyled(
        const text::StringView &text,
        const BlockStyle style,
        const text::EncodingErrorMode encodingErrors = text::EncodingErrorMode::Replace) {
        _data.appendCharacters(text, style.color(), style.attributes(), encodingErrors);
    }
    /// Materialize the current builder contents into an owned string copy.
    /// @return A copied terminal string.
    [[nodiscard]] auto toString() const -> BlockString {
        return BlockString::fromStorageWithDisplayWidth(_data.chars(), _data.displayWidth());
    }
    /// Move the built string out and reset the builder for fast reuse.
    /// @return The built terminal string.
    [[nodiscard]] auto takeString() -> BlockString {
        const auto displayWidth = _data.displayWidth();
        auto chars = std::move(_data.chars());
        _data = BlockStringData{};
        return BlockString::fromStorageWithDisplayWidth(std::move(chars), displayWidth);
    }

protected:
    BlockStringData _data; ///< Unique mutable storage for the built characters.
};

}
