// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UpdateSettings.hpp"

#include "BufferView.hpp"

namespace erbsland::cterm {

UpdateSettings::UpdateSettings(
    const bgeo::BlockSize minimumSize,
    const Block minimumSizeBackground,
    const bool showCropMarks,
    const Block cropMarkRight,
    const Block cropMarkBottom) noexcept :
    _minimumSize{minimumSize},
    _minimumSizeBackground{minimumSizeBackground},
    _showCropMarks{showCropMarks},
    _cropMarkRight{cropMarkRight},
    _cropMarkBottom{cropMarkBottom} {
}

auto UpdateSettings::minimumSize() const noexcept -> bgeo::BlockSize {
    return _minimumSize;
}

auto UpdateSettings::minimumSizeMark() const noexcept -> const Block & {
    return _minimumSizeBackground;
}

auto UpdateSettings::showCropMarks() const noexcept -> bool {
    return _showCropMarks;
}

auto UpdateSettings::cropMarkRight() const noexcept -> const Block & {
    return _cropMarkRight;
}

auto UpdateSettings::cropMarkBottomRight() const noexcept -> const Block & {
    return _cropMarkBottomRight;
}

auto UpdateSettings::cropMarkBottom() const noexcept -> const Block & {
    return _cropMarkBottom;
}

void UpdateSettings::setMinimumSize(const bgeo::BlockSize minimumSize) noexcept {
    _minimumSize = minimumSize;
}

auto UpdateSettings::minimumSizeBackground() const noexcept -> const Block & {
    return _minimumSizeBackground;
}

void UpdateSettings::setMinimumSizeBackground(const Block character) noexcept {
    _minimumSizeBackground = character;
}

auto UpdateSettings::minimumSizeMessage() const noexcept -> const BlockString & {
    return _minimumSizeMessage;
}

void UpdateSettings::setMinimumSizeMessage(BlockString message) noexcept {
    _minimumSizeMessage = std::move(message);
}

void UpdateSettings::setMinimumSizeMark(const Block minimumSizeMark) noexcept {
    _minimumSizeBackground = minimumSizeMark;
}

void UpdateSettings::setShowCropMarks(const bool showCropMarks) noexcept {
    _showCropMarks = showCropMarks;
}

void UpdateSettings::setCropMarkRight(const Block cropMarkRight) noexcept {
    _cropMarkRight = cropMarkRight;
}

void UpdateSettings::setCropMarkBottomRight(Block cropMarkBottomRight) noexcept {
    _cropMarkBottomRight = cropMarkBottomRight;
}

void UpdateSettings::setCropMarkBottom(const Block cropMarkBottom) noexcept {
    _cropMarkBottom = cropMarkBottom;
}

auto UpdateSettings::switchToAlternateBuffer() const noexcept -> bool {
    return _switchToAlternateBuffer;
}

void UpdateSettings::setSwitchToAlternateBuffer(const bool switchToAlternateBuffer) noexcept {
    _switchToAlternateBuffer = switchToAlternateBuffer;
}

void UpdateSettings::applyTo(BufferViewBase &view) const noexcept {
    view.setShowCropCharacters(_showCropMarks);
    view.setCropCharacter(bgeo::BlockDirection::South, _cropMarkBottom);
    view.setCropCharacter(bgeo::BlockDirection::SouthEast, _cropMarkBottomRight);
    view.setCropCharacter(bgeo::BlockDirection::East, _cropMarkRight);
}

auto UpdateSettings::defaultSettings() noexcept -> const UpdateSettings & {
    static UpdateSettings settings;
    return settings;
}

}
