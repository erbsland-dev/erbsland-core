// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringView.hpp"

#include "U16String.hpp"

#include "impl/U16StringComparisonTools.hpp"

namespace erbsland::text {

auto U16StringView::compare(const U16StringView &other, const CharCompareFn compareFn) const noexcept
    -> std::strong_ordering {
    return impl::U16StringComparisonTools{dataView()}.compare(other.dataView(), compareFn);
}

auto U16StringView::operator<=>(const U16StringView &other) const noexcept -> std::strong_ordering {
    return compare(other);
}

auto U16StringView::startsWith(const U16StringView &other, const CharCompareFn compareFn) const noexcept -> bool {
    return impl::U16StringComparisonTools{dataView()}.startsWith(other.dataView(), compareFn);
}

auto U16StringView::endsWith(const U16StringView &other, const CharCompareFn compareFn) const noexcept -> bool {
    return impl::U16StringComparisonTools{dataView()}.endsWith(other.dataView(), compareFn);
}

auto U16StringView::contains(const U16StringView &other, const CharCompareFn compareFn) const noexcept -> bool {
    return impl::U16StringComparisonTools{dataView()}.contains(other.dataView(), compareFn);
}

auto U16StringView::count(const U16StringView &text, const CharCompareFn compareFn) const noexcept
    -> unit::ElementCount {
    return impl::U16StringComparisonTools{dataView()}.count(text.dataView(), compareFn);
}

auto U16StringView::containsOneOf(const CharSet &characters) const noexcept -> bool {
    return impl::U16StringComparisonTools{dataView()}.containsOneOf(characters);
}

auto U16StringView::containsOnly(const CharSet &characters) const noexcept -> bool {
    return impl::U16StringComparisonTools{dataView()}.containsOnly(characters);
}

}
