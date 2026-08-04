// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "AnyString.hpp"

#include "AnyStringEditor.hpp"

#include "impl/AnyStringComparison.hpp"
#include "impl/StringTraits.hpp"

#include <type_traits>

namespace erbsland::text {

AnyString::AnyString(const AnyStringEditor &str) noexcept : AnyString(str.toAnyString()) {
}

auto AnyString::operator<=>(const AnyString &other) const noexcept -> std::strong_ordering {
    return compare(other);
}

auto AnyString::compare(const AnyString &other, const CharCompareFn compareFn) const noexcept -> std::strong_ordering {
    if (isEmpty()) {
        return other.isEmpty() ? std::strong_ordering::equal : std::strong_ordering::less;
    }
    if (other.isEmpty()) {
        return std::strong_ordering::greater;
    }
    return std::visit(
        [compareFn]<typename tLeft, typename tRight>(
            const tLeft &left, const tRight &right) noexcept -> std::strong_ordering {
            using Left = std::remove_cvref_t<tLeft>;
            using Right = std::remove_cvref_t<tRight>;
            if constexpr (!impl::AnyStringType<Left> || !impl::AnyStringType<Right>) {
                return std::strong_ordering::equal; // Empty values were handled before the visit.
            } else if constexpr (std::is_same_v<Left, Right>) {
                return left.compare(right, compareFn);
            } else {
                return impl::compareStringWidths(left, right, compareFn);
            }
        },
        _value,
        other._value);
}

}
