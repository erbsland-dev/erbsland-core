// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ReadableBuffer.hpp"

#include "Bitmap.hpp"

namespace erbsland::cterm {

auto ReadableBuffer::countDifferencesTo(const ReadableBuffer &other) const noexcept -> std::size_t {
    const auto thisRect = rect();
    const auto otherRect = other.rect();
    const auto overlap = thisRect & otherRect;
    auto differences = thisRect.size().area() + otherRect.size().area() - 2 * overlap.size().area();
    overlap.forEach([&](const block::Position pos) -> void {
        if (get(pos) != other.get(pos)) {
            differences += 1;
        }
    });
    return differences.toSizeT();
}

auto ReadableBuffer::toMask(const text::CharSet &characters, const bool invert) -> Bitmap {
    return toMaskImpl(characters, invert);
}

auto ReadableBuffer::toMask(std::initializer_list<text::Char> characters, const bool invert) -> Bitmap {
    return toMaskImpl(text::CharSet{characters}, invert);
}

auto ReadableBuffer::toMaskImpl(const text::CharSet &characters, const bool invert) -> Bitmap {
    const auto sourceRect = rect();
    auto bitmap = Bitmap{sourceRect.size()};
    if (characters.isEmpty()) {
        return bitmap;
    }
    sourceRect.forEach([&](const block::Position pos) -> void {
        auto isSet = characters.contains(get(pos).singleOrNull());
        if (invert) {
            isSet = !isSet;
        }
        bitmap.setPixel(pos - sourceRect.topLeft(), isSet);
    });
    return bitmap;
}

}
