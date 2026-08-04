// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BufferViewBase.hpp"
#include "CropEdges.hpp"

namespace erbsland::cterm {

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
