// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BufferViewBase.hpp"

#include "Buffer.hpp"

namespace erbsland::cterm {

auto BufferViewBase::size() const noexcept -> block::Size {
    return _viewRect.size();
}

auto BufferViewBase::rect() const noexcept -> block::Rectangle {
    return block::Rectangle{block::Position{0, 0}, size()};
}

auto BufferViewBase::clone() const -> WritableBufferPtr {
    auto buffer = std::make_shared<Buffer>(size());
    size().forEach([&](const block::Position pos) -> void { buffer->set(pos, get(pos)); });
    return buffer;
}

auto BufferViewBase::viewRect() const noexcept -> const block::Rectangle & {
    return _viewRect;
}

void BufferViewBase::setViewRect(block::Rectangle rect) noexcept {
    _viewRect = rect;
}

auto BufferViewBase::showCropCharacters() const noexcept -> bool {
    return _showCropCharacters;
}

void BufferViewBase::setShowCropCharacters(bool show) noexcept {
    _showCropCharacters = show;
}

auto BufferViewBase::cropCharacter(const block::Direction direction) const noexcept -> Block {
    if (static_cast<std::size_t>(direction) >= _cropCharacters.size()) {
        return Block{};
    }
    return _cropCharacters[direction];
}

void BufferViewBase::setCropCharacter(const block::Direction direction, const Block character) noexcept {
    if (static_cast<std::size_t>(direction) >= _cropCharacters.size()) {
        return;
    }
    _cropCharacters[direction] = character;
}

}
