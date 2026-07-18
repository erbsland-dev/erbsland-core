// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Block.hpp"
#include "../BlockCount.hpp"
#include "../BlockRange.hpp"

#include "../../mem/SharedData.hpp"
#include "../../mem/SharedDataPointer.hpp"
#include "../../text/EncodingErrorMode.hpp"
#include "../../text/String.hpp"
#include "../../text/u32/U32String.hpp"

#include <vector>

namespace erbsland::cterm::impl {

/// Shared immutable backing storage for `BlockStringEditor` and `BlockString`.
class BlockStringData final : public mem::SharedData {
public:
    using Storage = std::vector<Block>;
    constexpr static auto cNoCachedValue = -1;

public:
    /// Create one empty storage object.
    BlockStringData() noexcept = default;
    /// Create one shared storage object from a character buffer.
    /// @param chars The characters to store.
    explicit BlockStringData(Storage chars) noexcept;
    /// Create one shared storage object with an already known display width.
    /// @param chars The characters to store.
    /// @param displayWidth The cached display width for the full storage buffer.
    BlockStringData(Storage chars, int displayWidth) noexcept;

    // defaults
    ~BlockStringData() = default;
    BlockStringData(const BlockStringData &) = default;
    BlockStringData(BlockStringData &&) = default;
    auto operator=(const BlockStringData &) -> BlockStringData & = default;
    auto operator=(BlockStringData &&) -> BlockStringData & = default;

public:
    /// Access the stored characters.
    [[nodiscard]] auto chars() const noexcept -> const Storage & { return _chars; }
    /// Access the stored characters for a unique mutable owner.
    [[nodiscard]] auto chars() noexcept -> Storage & { return _chars; }
    /// Get the number of stored characters.
    [[nodiscard]] auto size() const noexcept -> std::size_t { return _chars.size(); }
    /// Get the cached display width for the full stored buffer.
    [[nodiscard]] auto displayWidth() const noexcept -> int { return _displayWidth; }
    /// Test if the cached display width is valid.
    [[nodiscard]] auto hasDisplayWidthCache() const noexcept -> bool { return _displayWidth != cNoCachedValue; }
    /// Reserve storage for at least the given number of characters.
    /// @param size The requested capacity.
    void reserve(std::size_t size);
    /// Clear the stored characters and reset the cached display width.
    void clear() noexcept;
    /// Append one terminal character and update the cached display width.
    /// @param character The character to append.
    void append(const Block &character) noexcept;
    /// Add one known display width contribution to the cached full-buffer width.
    /// @param displayWidth The width contribution to add.
    void addDisplayWidth(int displayWidth) noexcept;
    /// Mark the cached display width as invalid after mutable element access.
    void invalidateDisplayWidth() noexcept { _displayWidth = cNoCachedValue; }
    /// Materialize one character range into a new contiguous buffer.
    /// @param range The range to copy from the stored data.
    /// @return The copied characters.
    [[nodiscard]] auto copyChars(BlockRange range) const -> Storage;
    /// Append one decoded code point to the storage.
    /// @param codePoint The decoded code point.
    /// @param color The base color for newly appended characters.
    /// @param attributes The base attributes for newly appended characters.
    void appendCodePoint(text::Char codePoint, Color color, BlockAttributes attributes);
    /// Append text to the storage.
    /// @param text The source text.
    /// @param color The base color for newly appended characters.
    /// @param attributes The base attributes for newly appended characters.
    void appendCharacters(const text::U32String &text, Color color, BlockAttributes attributes) noexcept;
    /// Append UTF-8 text to the storage.
    /// @param text The source text.
    /// @param color The base color for newly appended characters.
    /// @param attributes The base attributes for newly appended characters.
    /// @param encodingErrorMode How malformed UTF-8 is handled.
    void appendCharacters(
        const text::String &text, Color color, BlockAttributes attributes, text::EncodingErrorMode encodingErrorMode);

public:
    /// Measure the terminal display width produced by UTF-8 text.
    /// @param text The source text.
    /// @param encodingErrorMode How malformed UTF-8 is handled.
    /// @return The display width of the accepted terminal characters.
    [[nodiscard]] static auto measureDisplayWidth(
        const text::String &text, text::EncodingErrorMode encodingErrorMode = text::EncodingErrorMode::Replace) -> int;
    /// Measure the terminal display width produced by text.
    /// @param text The source text.
    /// @return The display width of the accepted terminal characters.
    [[nodiscard]] static auto measureDisplayWidth(const text::U32String &text) -> int;

private:
    Storage _chars;      ///< The stored characters.
    int _displayWidth{}; ///< Cached display width for the full stored buffer.
};

using BlockStringDataPtr = mem::SharedDataPointer<BlockStringData, true>;

/// Create a shared immutable empty backing store.
[[nodiscard]] auto sharedEmptyBlockStringData() -> const BlockStringDataPtr &;

}
