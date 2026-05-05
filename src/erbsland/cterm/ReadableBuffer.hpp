// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Block.hpp"

#include "../bgeo/BlockRectangle.hpp"
#include "../bgeo/BlockSize.hpp"
#include "../text/CharSet.hpp"

#include <memory>

namespace erbsland::cterm {

class ReadableBuffer;
using ReadableBufferPtr = std::shared_ptr<ReadableBuffer>;
class Bitmap;
class WritableBuffer;
using WritableBufferPtr = std::shared_ptr<WritableBuffer>;

/// A readable buffer.
class ReadableBuffer {
public:
    virtual ~ReadableBuffer() = default;

public: // API
    /// Get the configured size of the buffer.
    /// @return The width and height of the buffer.
    [[nodiscard]] virtual auto size() const noexcept -> bgeo::BlockSize = 0;
    /// Get a rectangle representing this buffer.
    /// @return The rectangle for this buffer.
    [[nodiscard]] virtual auto rect() const noexcept -> bgeo::BlockRectangle = 0;
    /// Read the block stored at the given position.
    /// @param pos The coordinates within the buffer.
    /// @return A reference to the stored block.
    [[nodiscard]] virtual auto get(bgeo::BlockPosition pos) const noexcept -> const Block & = 0;
    /// Create a writeable copy of this buffer.
    /// This will copy every block from this buffer into a new independent instance.
    [[nodiscard]] virtual auto clone() const -> WritableBufferPtr = 0;

public: // convenience methods
    /// Count the differences from this to another buffer.
    /// If the size of `other` is smaller or larger than this buffer, the size change counts to the difference.
    /// @param other The other buffer to compare with.
    /// @return The number of blocks that differ between the two buffers.
    [[nodiscard]] virtual auto countDifferencesTo(const ReadableBuffer &other) const noexcept -> std::size_t;
    /// Create a mask from this buffer.
    /// All characters that match one of the given characters result in a pixel set in the mask.
    /// @param characters The characters to match. Only one code-point characters are supported.
    /// @param invert If true, *not*-matching characters result in a pixel set in the mask.
    /// @return A bitmap mask with the same size as this buffer.
    [[nodiscard]] auto toMask(const text::CharSet &characters, bool invert = false) -> Bitmap;
    /// @overload
    [[nodiscard]] auto toMask(std::initializer_list<text::Char> characters, bool invert = false) -> Bitmap;

protected: // implementation
    /// Implement `toMask()`.
    /// The public `toMask()` overloads forward to this method.
    /// @param characters The characters to match. Only one code-point characters are supported.
    /// @param invert If true, *not*-matching characters result in a pixel set in the mask.
    /// @return A bitmap mask with the same size as this buffer.
    [[nodiscard]] virtual auto toMaskImpl(const text::CharSet &characters, bool invert) -> Bitmap;
};

}
