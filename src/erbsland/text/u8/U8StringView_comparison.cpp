// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringView.hpp"

#include "U8String.hpp"

#include "impl/U8StringComparisonTools.hpp"

namespace erbsland::text {

auto U8StringView::compare(const U8StringView &other, const CharCompareFn compareFn) const noexcept
    -> std::strong_ordering {
    return impl::U8StringComparisonTools{dataView()}.compare(other.dataView(), compareFn);
}

auto U8StringView::operator<=>(const U8StringView &other) const noexcept -> std::strong_ordering {
    return compare(other);
}

auto U8StringView::startsWith(const U8StringView &other, const CharCompareFn compareFn) const noexcept -> bool {
    return impl::U8StringComparisonTools{dataView()}.startsWith(other.dataView(), compareFn);
}

auto U8StringView::endsWith(const U8StringView &other, const CharCompareFn compareFn) const noexcept -> bool {
    return impl::U8StringComparisonTools{dataView()}.endsWith(other.dataView(), compareFn);
}

auto U8StringView::contains(const U8StringView &other, const CharCompareFn compareFn) const noexcept -> bool {
    return impl::U8StringComparisonTools{dataView()}.contains(other.dataView(), compareFn);
}

auto U8StringView::count(const U8StringView &text, const CharCompareFn compareFn) const noexcept -> unit::ElementCount {
    return impl::U8StringComparisonTools{dataView()}.count(text.dataView(), compareFn);
}

auto U8StringView::containsOneOf(const CharSet &characters) const noexcept -> bool {
    return impl::U8StringComparisonTools{dataView()}.containsOneOf(characters);
}

auto U8StringView::containsOnly(const CharSet &characters) const noexcept -> bool {
    return impl::U8StringComparisonTools{dataView()}.containsOnly(characters);
}

}
