// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Bitmap.hpp"

#include "../unit/ByteIndex.hpp"
#include "../unit/ByteLength.hpp"

#include <algorithm>

namespace erbsland::cterm {

using namespace bgeo;

auto Bitmap::fromPattern(const std::initializer_list<text::String> rows) -> Bitmap {
    auto width = BlockCoordinate{0};
    for (const auto &row : rows) {
        width = std::max(width, BlockCoordinate{row.length().toSizeT()});
    }
    auto bitmap = Bitmap{BlockSize{width, BlockCoordinate{rows.size()}}};
    auto y = BlockCoordinate{0};
    for (const auto &row : rows) {
        for (auto x = BlockCoordinate{0}; x < BlockCoordinate{row.length().toSizeT()}; ++x) {
            const auto character = row[unit::ByteIndex::fromSizeT(x.toSizeT())];
            if (character != U'.' && character != U' ') {
                bitmap.setPixel(BlockPosition{x, y}, true);
            }
        }
        ++y;
    }
    return bitmap;
}

auto Bitmap::toPattern() const -> text::String {
    auto result = text::StringEditor{};
    result.reserve(unit::ByteLength::fromSizeT((_size.area() + _size.height()).toSizeT()));
    for (auto y = BlockCoordinate{0}; y < _size.height(); ++y) {
        for (auto x = BlockCoordinate{0}; x < _size.width(); ++x) {
            result.append(pixel(BlockPosition{x, y}) ? U'#' : U'.');
        }
        result.append(U'\n');
    }
    return result;
}

auto Bitmap::pixel(const BlockPosition pos) const noexcept -> bool {
    if (!_size.contains(pos)) {
        return false;
    }
    return _data[_size.index(pos)];
}

auto Bitmap::pixelQuad(const BlockPosition pos) const noexcept -> uint8_t {
    const auto base = BlockPosition{pos.x() * 2, pos.y() * 2};
    constexpr auto positions = std::array<std::pair<BlockPosition, uint8_t>, 4U>{
        std::pair{BlockPosition{0, 0}, static_cast<uint8_t>(0b0001U)},
        std::pair{BlockPosition{1, 0}, static_cast<uint8_t>(0b0010U)},
        std::pair{BlockPosition{0, 1}, static_cast<uint8_t>(0b0100U)},
        std::pair{BlockPosition{1, 1}, static_cast<uint8_t>(0b1000U)},
    };
    uint8_t result = 0;
    for (const auto &[delta, mask] : positions) {
        if (pixel(base + delta)) {
            result |= mask;
        }
    }
    return result;
}

auto Bitmap::pixelCardinal(BlockPosition pos) const noexcept -> uint8_t {
    constexpr auto positions = std::array<std::pair<BlockPosition, uint8_t>, 4U>{
        std::pair{BlockPosition{1, 0}, static_cast<uint8_t>(0b0001U)},
        std::pair{BlockPosition{0, 1}, static_cast<uint8_t>(0b0010U)},
        std::pair{BlockPosition{-1, 0}, static_cast<uint8_t>(0b0100U)},
        std::pair{BlockPosition{0, -1}, static_cast<uint8_t>(0b1000U)},
    };
    uint8_t result = 0;
    for (const auto &[delta, mask] : positions) {
        if (pixel(pos + delta)) {
            result |= mask;
        }
    }
    return result;
}

auto Bitmap::pixelRing(const BlockPosition pos) const noexcept -> uint8_t {
    constexpr auto positions = std::array<std::pair<BlockPosition, uint8_t>, 8U>{
        std::pair{BlockPosition{1, 0}, static_cast<uint8_t>(0b00000001U)},
        std::pair{BlockPosition{1, 1}, static_cast<uint8_t>(0b00000010U)},
        std::pair{BlockPosition{0, 1}, static_cast<uint8_t>(0b00000100U)},
        std::pair{BlockPosition{-1, 1}, static_cast<uint8_t>(0b00001000U)},
        std::pair{BlockPosition{-1, 0}, static_cast<uint8_t>(0b00010000U)},
        std::pair{BlockPosition{-1, -1}, static_cast<uint8_t>(0b00100000U)},
        std::pair{BlockPosition{0, -1}, static_cast<uint8_t>(0b01000000U)},
        std::pair{BlockPosition{1, -1}, static_cast<uint8_t>(0b10000000U)},
    };
    uint8_t result = 0;
    for (const auto &[delta, mask] : positions) {
        if (pixel(pos + delta)) {
            result |= mask;
        }
    }
    return result;
}

auto Bitmap::boundingRect(bool value) const noexcept -> BlockRectangle {
    const BlockCoordinate w = _size.width();
    const BlockCoordinate h = _size.height();

    auto rowHasValue = [&](BlockCoordinate y) noexcept -> bool {
        for (auto x = BlockCoordinate{0}; x < w; ++x) {
            if (pixel({x, y}) == value) {
                return true;
            }
        }
        return false;
    };

    auto colHasValue = [&](BlockCoordinate x) noexcept -> bool {
        for (auto y = BlockCoordinate{0}; y < h; ++y) {
            if (pixel({x, y}) == value) {
                return true;
            }
        }
        return false;
    };

    auto top = BlockCoordinate{0};
    while (top < h && !rowHasValue(top)) {
        ++top;
    }
    if (top == h) {
        return BlockRectangle{};
    }

    auto bottom = h - 1;
    while (bottom > top && !rowHasValue(bottom)) {
        --bottom;
    }

    auto left = BlockCoordinate{0};
    while (left < w && !colHasValue(left)) {
        ++left;
    }

    auto right = w - 1;
    while (right > left && !colHasValue(right)) {
        --right;
    }

    return BlockRectangle{left, top, right - left + 1, bottom - top + 1};
}

auto Bitmap::pixelCount(const bool value) const noexcept -> std::size_t {
    std::size_t result = 0;
    _size.forEach([&](const BlockPosition pos) -> void {
        if (pixel(pos) == value) {
            ++result;
        }
    });
    return result;
}

void Bitmap::setPixel(const BlockPosition pos, const bool value) noexcept {
    if (!_size.contains(pos)) {
        return;
    }
    _data[_size.index(pos)] = value;
}

void Bitmap::flipHorizontal() noexcept {
    for (auto y = BlockCoordinate{0}; y < _size.height(); ++y) {
        for (auto x = BlockCoordinate{0}; x < _size.width() / 2; ++x) {
            const auto p1 = BlockPosition{x, y};
            const auto p2 = BlockPosition{_size.width() - 1 - x, y};
            if (p1 != p2) {
                const auto pixel1 = static_cast<bool>(pixelRef(p1));
                const auto pixel2 = static_cast<bool>(pixelRef(p2));
                pixelRef(p1) = pixel2;
                pixelRef(p2) = pixel1;
            }
        }
    }
}

void Bitmap::invert() noexcept {
    _size.forEach([this](const BlockPosition pos) -> void { pixelRef(pos) = !pixelRef(pos); });
}

auto Bitmap::inverted() const noexcept -> Bitmap {
    auto result = *this;
    result.invert();
    return result;
}

auto Bitmap::outlined() const noexcept -> Bitmap {
    return fromFunction(_size, [&](const BlockPosition pos) -> bool { return !pixel(pos) && pixelRing(pos) != 0U; });
}

auto Bitmap::expanded(const BlockMargins margins, const bool value) const noexcept -> Bitmap {
    const auto newSize = BlockSize{_size.width() + margins.horizontalDelta(), _size.height() + margins.verticalDelta()};
    if (newSize.width() <= 0 || newSize.height() <= 0) {
        return {};
    }
    auto result = Bitmap{newSize};
    if (value) {
        result.fillRect(result.rect(), true);
    }
    const auto sourceToTargetOffset = BlockPosition{margins.left(), margins.top()};
    const auto targetRectInSourceCoordinates = BlockRectangle{
        BlockPosition{-sourceToTargetOffset.x(), -sourceToTargetOffset.y()},
        newSize,
    };
    const auto copyRect = targetRectInSourceCoordinates & rect();
    copyRect.forEach([&](const BlockPosition pos) -> void { result.setPixel(pos + sourceToTargetOffset, pixel(pos)); });
    return result;
}

void Bitmap::fillRect(const BlockRectangle rect, const bool value) noexcept {
    if (!this->rect().overlaps(rect)) {
        return;
    }
    (this->rect() & rect).forEach([&](const BlockPosition pos) -> void { pixelRef(pos) = value; });
}

void Bitmap::floodFill(const BlockPosition pos, const bool value) noexcept {
    if (!_size.contains(pos) || pixelRef(pos) == value) {
        return;
    }
    std::vector<BlockPosition> queue;
    queue.reserve(100);
    queue.push_back(pos);
    while (!queue.empty()) {
        const auto current = queue.back();
        queue.pop_back();
        if (pixelRef(current) != value) {
            pixelRef(current) = value;
        }
        for (const auto neighbor : current.cardinalFour()) {
            if (_size.contains(neighbor) && pixelRef(neighbor) != value) {
                queue.push_back(neighbor);
            }
        }
    }
}

}
