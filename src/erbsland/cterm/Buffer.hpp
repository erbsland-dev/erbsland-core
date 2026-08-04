// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Buffer_fwd.hpp"
#include "WritableBuffer.hpp"

#include "../text/String.hpp"

#include <vector>

namespace erbsland::cterm {

/// A mutable 2D buffer storing characters and colors for rendering.
///
/// Handling of non-1-width blocks:
/// - Blocks with zero display width are ignored.
/// - Blocks with a display width of 2 will overwrite two adjacent cells.
///   - The first cell will contain the set character, the next cell a zero-width (empty) character.
///   - Both cells will have the same color.
///   - A 2-width block at the right edge is ignored.
/// - Blocks with a display width > 2 are ignored.
class Buffer final : public WritableBuffer {
public:
    /// Largest buffer size accepted for this buffer.
    constexpr static auto cMaximumSize = bgeo::BlockSize{10'000, 10'000};
    /// Smallest valid buffer size.
    constexpr static auto cMinimumSize = bgeo::BlockSize{1, 1};

    using WritableBuffer::drawBitmap;
    using WritableBuffer::drawBlockText;
    using WritableBuffer::fill;
    using WritableBuffer::get;
    using WritableBuffer::resize;
    using WritableBuffer::set;

public:
    /// Creates a 1x1 buffer filled with a space.
    /// Usually only used as a placeholder until resized.
    Buffer();

    /// Construct a buffer with the given size and fill it with an initial block.
    /// @param size The dimensions of the buffer. bgeo::BlockSize must be at least 1x1.
    /// @param fillChar The optional fill character for the buffer.
    /// @throws err::ParameterError if size is invalid.
    explicit Buffer(bgeo::BlockSize size, Block fillChar = Block::space());

    // defaults
    ~Buffer() override = default;
    Buffer(const Buffer &) = default;
    Buffer(Buffer &&) = default;
    auto operator=(const Buffer &) -> Buffer & = default;
    auto operator=(Buffer &&) -> Buffer & = default;

public: // implement ReadableBuffer
    [[nodiscard]] auto size() const noexcept -> bgeo::BlockSize override;
    [[nodiscard]] auto rect() const noexcept -> bgeo::BlockRectangle override;
    [[nodiscard]] auto get(bgeo::BlockPosition pos) const noexcept -> const Block & override;
    [[nodiscard]] auto clone() const -> WritableBufferPtr override;

public: // implement WritableBuffer
    void resize(bgeo::BlockSize newSize) override;
    void resize(bgeo::BlockSize size, BufferResizeMode mode, Block fillChar) override;
    void set(bgeo::BlockPosition pos, const Block &block) noexcept override;
    void setAndResizeFrom(const ReadableBuffer &other) override;

public: // faster implementations
    /// Fill/clear the buffer with the given character.
    /// @param fillBlock The block to use to fill the buffer.
    void fill(const Block &fillBlock) noexcept override;

public: // builders
    /// Creates a buffer from the lines in a string.
    /// This function splits the given string into lines and creates a buffer with a matching size.
    /// @param text The string to split into lines and create a buffer from. Must not be empty.
    /// @return A buffer containing the lines from the input string.
    [[nodiscard]] static auto fromLinesInString(const BlockString &text) -> Buffer;

    /// Creates a buffer from the lines in a string.
    /// @param lines The lines to create the buffer from. Must not be empty.
    /// @return A buffer containing the lines from the input string.
    [[nodiscard]] static auto fromLines(const BlockStringLines &lines) -> Buffer;

public: // compatibility
    /// Draw text into a rectangle using the legacy parameter order.
    /// @param text The text to render.
    /// @param alignment The alignment inside the rectangle.
    /// @param rect The target rectangle.
    /// @param color The text color.
    /// @param animationCycle Animation cycle for animated text.
    /// Invalid UTF-8 bytes are replaced with the Unicode replacement character.
    void drawBlockText(
        const text::String &text,
        bgeo::Alignment alignment,
        bgeo::BlockRectangle rect,
        Color color = {},
        std::size_t animationCycle = 0);

private:
    /// Validate the buffer size and return it if it is valid, otherwise throw an exception.
    static auto validatedBufferSize(bgeo::BlockSize size) -> bgeo::BlockSize;

private:
    bgeo::BlockSize _size{cMinimumSize};
    std::vector<Block> _data{Block::space()};
};

}
