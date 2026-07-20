// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Char.hpp"
#include "../CharCompareFn.hpp"
#include "../StringSide.hpp"

#include <compare>

namespace erbsland::text::impl {

/// Compare strings with different code-unit widths by decoded code point.
/// @tparam tLeft The left read-only string type.
/// @tparam tRight The right read-only string type.
/// @param left The left string.
/// @param right The right string.
/// @param compareFn The optional character comparison function.
/// @return A three-way comparison result.
/// @tested{AnyStringTest}
template <typename tLeft, typename tRight>
[[nodiscard]] auto compareStringWidths(
    const tLeft &left, const tRight &right, const CharCompareFn compareFn = {}) noexcept -> std::strong_ordering {
    auto leftIndex = left.indexAt(StringSide::Front);
    auto rightIndex = right.indexAt(StringSide::Front);
    while (true) {
        const auto leftCharacter = left.readCharAndAdvance(leftIndex);
        const auto rightCharacter = right.readCharAndAdvance(rightIndex);
        if (leftCharacter.isEndOfData()) {
            return rightCharacter.isEndOfData() ? std::strong_ordering::equal : std::strong_ordering::less;
        }
        if (rightCharacter.isEndOfData()) {
            return std::strong_ordering::greater;
        }
        const auto result =
            compareFn == nullptr ? leftCharacter <=> rightCharacter : compareFn(leftCharacter, rightCharacter);
        if (result != std::strong_ordering::equal) {
            return result;
        }
    }
}

}
