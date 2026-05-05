// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ColorSequence.hpp"

#include <algorithm>

namespace erbsland::cterm {

ColorSequence::ColorSequence(const Color color, const std::size_t count) noexcept {
    add(color, count);
}

ColorSequence::ColorSequence(const std::initializer_list<Color> colors) noexcept {
    for (const auto color : colors) {
        add(color);
    }
}

ColorSequence::ColorSequence(const std::initializer_list<Entry> entries) noexcept {
    for (const auto &entry : entries) {
        add(entry.color, entry.count);
    }
}

void ColorSequence::add(const Color color, const std::size_t count) noexcept {
    if (count == 0) {
        return;
    }
    _entries.push_back({color, count});
    _sequenceLength += count;
}

auto ColorSequence::color(const std::size_t index) const noexcept -> Color {
    if (_sequenceLength == 0) {
        return {};
    }
    return colorAtEffectiveIndex(index % _sequenceLength);
}

auto ColorSequence::colorNormalized(const double normalized) const noexcept -> Color {
    if (_sequenceLength == 0) {
        return {};
    }
    const auto clamped = std::clamp(normalized, 0.0, 1.0);
    const auto maxIndex = _sequenceLength - 1;
    const auto scaledIndex = static_cast<std::size_t>(clamped * static_cast<double>(maxIndex));
    return color(scaledIndex);
}

auto ColorSequence::colorAtEffectiveIndex(std::size_t index) const noexcept -> Color {
    for (const auto &entry : _entries) {
        if (index < entry.count) {
            return entry.color;
        }
        index -= entry.count;
    }
    return {};
}

}
