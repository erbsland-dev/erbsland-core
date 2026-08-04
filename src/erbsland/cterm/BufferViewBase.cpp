// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BufferViewBase.hpp"

#include "Buffer.hpp"

namespace erbsland::cterm {

auto BufferViewBase::size() const noexcept -> bgeo::BlockSize {
    return _viewRect.size();
}

auto BufferViewBase::rect() const noexcept -> bgeo::BlockRectangle {
    return bgeo::BlockRectangle{bgeo::BlockPosition{0, 0}, size()};
}

auto BufferViewBase::clone() const -> WritableBufferPtr {
    auto buffer = std::make_shared<Buffer>(size());
    size().forEach([&](const bgeo::BlockPosition pos) -> void { buffer->set(pos, get(pos)); });
    return buffer;
}

auto BufferViewBase::viewRect() const noexcept -> const bgeo::BlockRectangle & {
    return _viewRect;
}

void BufferViewBase::setViewRect(bgeo::BlockRectangle rect) noexcept {
    _viewRect = rect;
}

auto BufferViewBase::showCropCharacters() const noexcept -> bool {
    return _showCropCharacters;
}

void BufferViewBase::setShowCropCharacters(bool show) noexcept {
    _showCropCharacters = show;
}

auto BufferViewBase::cropCharacter(const bgeo::BlockDirection direction) const noexcept -> Block {
    if (static_cast<std::size_t>(direction) >= _cropCharacters.size()) {
        return Block{};
    }
    return _cropCharacters[direction];
}

void BufferViewBase::setCropCharacter(const bgeo::BlockDirection direction, const Block character) noexcept {
    if (static_cast<std::size_t>(direction) >= _cropCharacters.size()) {
        return;
    }
    _cropCharacters[direction] = character;
}

}
