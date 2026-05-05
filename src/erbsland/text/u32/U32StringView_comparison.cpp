// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringView.hpp"

#include "U32String.hpp"

#include "impl/U32StringComparisonTools.hpp"

namespace erbsland::text {

auto U32StringView::compare(const U32StringView &other, const CharCompareFn compareFn) const noexcept
    -> std::strong_ordering {
    return impl::U32StringComparisonTools{dataView()}.compare(other.dataView(), compareFn);
}

auto U32StringView::operator<=>(const U32StringView &other) const noexcept -> std::strong_ordering {
    return compare(other);
}

auto U32StringView::startsWith(const U32StringView &other, const CharCompareFn compareFn) const noexcept -> bool {
    return impl::U32StringComparisonTools{dataView()}.startsWith(other.dataView(), compareFn);
}

auto U32StringView::endsWith(const U32StringView &other, const CharCompareFn compareFn) const noexcept -> bool {
    return impl::U32StringComparisonTools{dataView()}.endsWith(other.dataView(), compareFn);
}

auto U32StringView::contains(const U32StringView &other, const CharCompareFn compareFn) const noexcept -> bool {
    return impl::U32StringComparisonTools{dataView()}.contains(other.dataView(), compareFn);
}

auto U32StringView::count(const U32StringView &text, const CharCompareFn compareFn) const noexcept
    -> unit::ElementCount {
    return impl::U32StringComparisonTools{dataView()}.count(text.dataView(), compareFn);
}

auto U32StringView::containsOneOf(const CharSet &characters) const noexcept -> bool {
    return impl::U32StringComparisonTools{dataView()}.containsOneOf(characters);
}

auto U32StringView::containsOnly(const CharSet &characters) const noexcept -> bool {
    return impl::U32StringComparisonTools{dataView()}.containsOnly(characters);
}

}
