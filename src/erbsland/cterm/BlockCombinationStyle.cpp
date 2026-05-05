// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BlockCombinationStyle.hpp"

#include "impl/CommonBoxFrameBlockCombinationStyle.hpp"

#include "../unit/CpIndex.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace erbsland::cterm {

auto BlockCombinationStyle::combine([[maybe_unused]] const Block &current, const Block &overlay) const noexcept
    -> Block {
    return overlay;
}

auto BlockCombinationStyle::combine(
    [[maybe_unused]] const std::array<const Block *, 9> &current, const Block &overlay) const noexcept -> Block {
    return overlay;
}

auto BlockCombinationStyle::overwrite() noexcept -> const BlockCombinationStylePtr & {
    static const auto style = std::make_shared<BlockCombinationStyle>();
    return style;
}

auto BlockCombinationStyle::colorOverlay() noexcept -> const BlockCombinationStylePtr & {
    static const BlockCombinationStylePtr style = std::make_shared<SimpleBlockCombinationStyle>();
    return style;
}

auto BlockCombinationStyle::commonBoxFrame() noexcept -> const BlockCombinationStylePtr & {
    static const BlockCombinationStylePtr style = std::make_shared<CommonBoxFrameBlockCombinationStyle>();
    return style;
}

SimpleBlockCombinationStyle::SimpleBlockCombinationStyle(Map map) noexcept : _map{std::move(map)} {
}

auto SimpleBlockCombinationStyle::combine(const Block &current, const Block &overlay) const noexcept -> Block {
    auto result = Block{};
    auto key = current.charStr();
    key.append(overlay.charStr());
    if (const auto combined = map().get(key)) {
        result = Block{*combined};
    } else {
        result = Block{overlay.charStr()};
    }
    result.setStyle(result.style().withBase(current.style()).withOverlay(overlay.style()));
    return result;
}

auto SimpleBlockCombinationStyle::map() const noexcept -> const Map & {
    return _map;
}

void SimpleBlockCombinationStyle::setMap(Map map) noexcept {
    _map = std::move(map);
}

void SimpleBlockCombinationStyle::add(
    const text::StringView &current, const text::StringView &overlay, const text::StringView &combined) noexcept {
    auto key = current.copy();
    key.append(overlay);
    _map.set(key, combined.copy());
}

MatrixBlockCombinationStyle::MatrixBlockCombinationStyle(
    const text::U32StringView &characters, const std::span<const uint8_t> resultMatrix) :
    _characters{characters.copy()}, _resultMatrix{resultMatrix.begin(), resultMatrix.end()} {
    if (_characters.length().toSizeT() > std::numeric_limits<uint8_t>::max()) {
        throw std::invalid_argument{"MatrixBlockCombinationStyle supports at most 255 characters."};
    }
    const auto characterCount = _characters.length().toSizeT();
    const auto expectedMatrixSize = characterCount * characterCount;
    if (_resultMatrix.size() != expectedMatrixSize) {
        throw std::invalid_argument{
            "MatrixBlockCombinationStyle result matrix size does not match the character count."};
    }
    if (_characters.isEmpty()) {
        return;
    }
    auto minimumCodePoint = _characters[unit::CpIndex{0U}];
    auto maximumCodePoint = minimumCodePoint;
    for (auto index = std::size_t{1U}; index < characterCount; ++index) {
        const auto character = _characters[unit::CpIndex::fromSizeT(index)];
        minimumCodePoint = std::min(minimumCodePoint, character);
        maximumCodePoint = std::max(maximumCodePoint, character);
    }
    _lookupBase = minimumCodePoint;
    const auto lookupSize =
        static_cast<std::size_t>(maximumCodePoint.toRawValue() - minimumCodePoint.toRawValue() + 1U);
    _characterIndexByCodePoint.assign(lookupSize, cUnsupportedIndex);
    for (std::size_t index = 0; index < characterCount; ++index) {
        const auto offset = static_cast<std::size_t>(
            _characters[unit::CpIndex::fromSizeT(index)].toRawValue() - _lookupBase.toRawValue());
        _characterIndexByCodePoint[offset] = static_cast<uint8_t>(index);
    }
}

auto MatrixBlockCombinationStyle::combine(const Block &current, const Block &overlay) const noexcept -> Block {
    auto result = overlay;
    const auto currentIndex = lookupIndex(current.mainCodePoint());
    const auto overlayIndex = lookupIndex(overlay.mainCodePoint());
    if (currentIndex != cUnsupportedIndex && overlayIndex != cUnsupportedIndex) {
        const auto matrixSize = _characters.length().toSizeT();
        const auto matrixIndex =
            static_cast<std::size_t>(currentIndex) * matrixSize + static_cast<std::size_t>(overlayIndex);
        if (matrixIndex < _resultMatrix.size()) {
            const auto resultIndex = _resultMatrix[matrixIndex];
            if (resultIndex < _characters.length().toSizeT()) {
                result = Block{_characters[unit::CpIndex::fromSizeT(resultIndex)]};
            }
        }
    }
    result.setStyle(result.style().withBase(current.style()).withOverlay(overlay.style()));
    return result;
}

auto MatrixBlockCombinationStyle::lookupIndex(const text::Char codePoint) const noexcept -> uint8_t {
    const auto rawCodePoint = codePoint.toRawValue();
    if (_characterIndexByCodePoint.empty() || rawCodePoint < _lookupBase.toRawValue()) {
        return cUnsupportedIndex;
    }
    const auto offset = static_cast<std::size_t>(rawCodePoint - _lookupBase.toRawValue());
    if (offset >= _characterIndexByCodePoint.size()) {
        return cUnsupportedIndex;
    }
    return _characterIndexByCodePoint[offset];
}

}
