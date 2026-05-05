// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Block.hpp"
#include "CropEdges.hpp"
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

/// A view that uses a shared pointer to the content.
class BufferView final : public BufferViewBase {
public:
    /// Create an empty view.
    /// This creates a 1x1 view that returns the 'bgeo::BlockDirection::None' character.
    BufferView() = default;
    /// Create an empty view of a given size.
    /// This creates a view that returns the 'bgeo::BlockDirection::None' character.
    /// @param viewSize The size of the view.
    explicit BufferView(const bgeo::BlockSize viewSize) noexcept :
        BufferViewBase{bgeo::BlockRectangle{bgeo::BlockPosition{0, 0}, viewSize}} {};
    /// Create a view of the given content, with a given size.
    /// The view shares the top-left corner with the buffer.
    /// @param content The buffer to create the view from.
    /// @param viewSize The size of the view.
    BufferView(ReadableBufferPtr content, const bgeo::BlockSize viewSize) noexcept :
        BufferViewBase{bgeo::BlockRectangle{bgeo::BlockPosition{0, 0}, viewSize}}, _content{std::move(content)} {}
    /// Create a view of the given content.
    /// @param content The buffer to create the view from.
    /// @param viewRect The rectangle of the view.
    BufferView(ReadableBufferPtr content, const bgeo::BlockRectangle viewRect) noexcept :
        BufferViewBase{viewRect}, _content{std::move(content)} {}

    // defaults
    ~BufferView() override = default;
    BufferView(const BufferView &) = default;
    BufferView(BufferView &&) = default;
    auto operator=(const BufferView &) -> BufferView & = default;
    auto operator=(BufferView &&) -> BufferView & = default;

public: // implement ReadableBuffer
    [[nodiscard]] auto get(const bgeo::BlockPosition pos) const noexcept -> const Block & override {
        if (_content == nullptr || _viewRect.size() == bgeo::BlockSize{0, 0}) {
            return _cropCharacters[bgeo::BlockDirection::None];
        }
        const auto translatedPos = pos + _viewRect.topLeft();
        if (_content->size().contains(translatedPos)) {
            if (_showCropCharacters) {
                const auto cropEdges = CropEdges::fromView(_viewRect, _content->rect());
                const auto cropDirection = cropEdges.edgeForView(translatedPos, _viewRect);
                if (cropDirection != bgeo::BlockDirection::None) {
                    return _cropCharacters[cropDirection];
                }
            }
            return _content->get(translatedPos);
        }
        return _cropCharacters[bgeo::BlockDirection::None];
    }

public:
    /// Access the content.
    [[nodiscard]] auto content() const noexcept -> const ReadableBufferPtr &;
    /// Replace the content.
    void setContent(ReadableBufferPtr buffer) noexcept;

private:
    ReadableBufferPtr _content;
};

/// A view that uses a reference to the content.
/// This view is to use as a thin temporary wrapper on the stack.
class BufferConstRefView final : public BufferViewBase {
public:
    /// Create a view of the given content, with a given size.
    /// The view shares the top-left corner with the buffer.
    /// @param content A reference to the content buffer.
    /// @param viewSize The size of the view.
    BufferConstRefView(const ReadableBuffer &content, const bgeo::BlockSize viewSize) noexcept :
        BufferViewBase{bgeo::BlockRectangle{bgeo::BlockPosition{0, 0}, viewSize}}, _buffer{content} {}
    /// Create a view of the given content.
    /// @param content A reference to the content buffer.
    /// @param viewRect The rectangle of the view.
    BufferConstRefView(const ReadableBuffer &content, const bgeo::BlockRectangle viewRect) noexcept :
        BufferViewBase{viewRect}, _buffer{content} {}

    // defaults
    ~BufferConstRefView() override = default;

    // delete copy/assign/move
    BufferConstRefView(const BufferConstRefView &) = delete;
    BufferConstRefView(BufferConstRefView &&) = delete;
    auto operator=(const BufferConstRefView &) -> BufferConstRefView & = delete;
    auto operator=(BufferConstRefView &&) -> BufferConstRefView & = delete;

public: // implement ReadableBuffer
    [[nodiscard]] auto get(const bgeo::BlockPosition pos) const noexcept -> const Block & override {
        if (_viewRect.size() == bgeo::BlockSize{0, 0}) {
            return _cropCharacters[bgeo::BlockDirection::None];
        }
        const auto translatedPos = pos + _viewRect.topLeft();
        if (_buffer.size().contains(translatedPos)) {
            if (_showCropCharacters) {
                const auto cropEdges = CropEdges::fromView(_viewRect, _buffer.rect());
                const auto cropDirection = cropEdges.edgeForView(translatedPos, _viewRect);
                if (cropDirection != bgeo::BlockDirection::None) {
                    return _cropCharacters[cropDirection];
                }
            }
            return _buffer.get(translatedPos);
        }
        return _cropCharacters[bgeo::BlockDirection::None];
    }

private:
    const ReadableBuffer &_buffer; ///< The reference to the buffer.
};

}
