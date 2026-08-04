// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Block.hpp"
#include "BufferViewBase_fwd.hpp"
#include "ReadableBuffer.hpp"

#include "../bgeo/BlockDirection.hpp"

namespace erbsland::cterm {

/// The base class for all buffer views.
class BufferViewBase : public ReadableBuffer {
public:
    /// Create a buffer view with the given visible rectangle in the source buffer.
    /// @param viewRectangle The rectangle of the underlying content that shall be exposed through the view.
    explicit BufferViewBase(const bgeo::BlockRectangle viewRectangle) noexcept : _viewRect{viewRectangle} {}

    // defaults
    BufferViewBase() = default;
    ~BufferViewBase() override = default;
    BufferViewBase(const BufferViewBase &) = default;
    BufferViewBase(BufferViewBase &&) = default;
    auto operator=(const BufferViewBase &) -> BufferViewBase & = default;
    auto operator=(BufferViewBase &&) -> BufferViewBase & = default;

public: // implement ReadableBuffer
    [[nodiscard]] auto size() const noexcept -> bgeo::BlockSize override;
    [[nodiscard]] auto rect() const noexcept -> bgeo::BlockRectangle override;
    [[nodiscard]] auto clone() const -> WritableBufferPtr override;

public:
    /// Get the rectangle in the underlying content that is currently visible through this view.
    [[nodiscard]] auto viewRect() const noexcept -> const bgeo::BlockRectangle &;
    /// Set the rectangle in the underlying content that shall be visible through this view.
    /// @param rect The new source rectangle for the view.
    void setViewRect(bgeo::BlockRectangle rect) noexcept;
    /// Test whether crop indicator characters are shown when the view is clipped by the source buffer.
    [[nodiscard]] auto showCropCharacters() const noexcept -> bool;
    /// Enable or disable crop indicator characters.
    /// @param show `true` to render crop indicators inside the view.
    void setShowCropCharacters(bool show) noexcept;
    /// Get the crop indicator character for one direction.
    /// @param direction The direction for which to retrieve the crop indicator.
    /// @return The configured crop character, or an empty character for an invalid direction.
    [[nodiscard]] auto cropCharacter(bgeo::BlockDirection direction) const noexcept -> Block;
    /// Set the crop indicator character for one direction.
    /// @param direction The direction to update.
    /// @param character The new crop indicator character.
    void setCropCharacter(bgeo::BlockDirection direction, Block character) noexcept;

protected:
    bgeo::BlockRectangle _viewRect{0, 0, 1, 1};
    bool _showCropCharacters = false;
    std::array<Block, bgeo::BlockDirection::cCount> _cropCharacters{
        Block{U' '},
        Block{U'▲'},
        Block{U'◥'},
        Block{U'▶'},
        Block{U'◢'},
        Block{U'▼'},
        Block{U'◣'},
        Block{U'◀'},
        Block{U'◤'},
    };
};

}
