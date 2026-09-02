// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BufferViewBase.hpp"
#include "CropEdges.hpp"

namespace erbsland::cterm {

/// A view that uses a shared pointer to the content.
class BufferView final : public BufferViewBase {
public:
    /// Create an empty view.
    /// This creates a 1x1 view that returns the 'block::Direction::None' character.
    BufferView() = default;
    /// Create an empty view of a given size.
    /// This creates a view that returns the 'block::Direction::None' character.
    /// @param viewSize The size of the view.
    explicit BufferView(const block::Size viewSize) noexcept :
        BufferViewBase{block::Rectangle{block::Position{0, 0}, viewSize}} {};
    /// Create a view of the given content, with a given size.
    /// The view shares the top-left corner with the buffer.
    /// @param content The buffer to create the view from.
    /// @param viewSize The size of the view.
    BufferView(ReadableBufferPtr content, const block::Size viewSize) noexcept :
        BufferViewBase{block::Rectangle{block::Position{0, 0}, viewSize}}, _content{std::move(content)} {}
    /// Create a view of the given content.
    /// @param content The buffer to create the view from.
    /// @param viewRect The rectangle of the view.
    BufferView(ReadableBufferPtr content, const block::Rectangle viewRect) noexcept :
        BufferViewBase{viewRect}, _content{std::move(content)} {}

    // defaults
    ~BufferView() override = default;
    BufferView(const BufferView &) = default;
    BufferView(BufferView &&) = default;
    auto operator=(const BufferView &) -> BufferView & = default;
    auto operator=(BufferView &&) -> BufferView & = default;

public: // implement ReadableBuffer
    [[nodiscard]] auto get(const block::Position pos) const noexcept -> const Block & override {
        if (_content == nullptr || _viewRect.size() == block::Size{0, 0}) {
            return _cropCharacters[block::Direction::None];
        }
        const auto translatedPos = pos + _viewRect.topLeft();
        if (_content->size().contains(translatedPos)) {
            if (_showCropCharacters) {
                const auto cropEdges = CropEdges::fromView(_viewRect, _content->rect());
                const auto cropDirection = cropEdges.edgeForView(translatedPos, _viewRect);
                if (cropDirection != block::Direction::None) {
                    return _cropCharacters[cropDirection];
                }
            }
            return _content->get(translatedPos);
        }
        return _cropCharacters[block::Direction::None];
    }

public:
    /// Access the content.
    [[nodiscard]] auto content() const noexcept -> const ReadableBufferPtr &;
    /// Replace the content.
    void setContent(ReadableBufferPtr buffer) noexcept;

private:
    ReadableBufferPtr _content;
};

}
