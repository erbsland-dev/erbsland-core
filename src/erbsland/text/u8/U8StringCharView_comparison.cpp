// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringCharView.hpp"

#include "U8StringView.hpp"

#include "impl/U8StringComparisonTools.hpp"

namespace erbsland::text {

auto U8StringCharView::isEmpty() const noexcept -> bool {
    return length().isZero();
}

auto U8StringCharView::startsWith(const U8StringView &other, const CharCompareFn compareFn) const noexcept -> bool {
    return impl::U8StringComparisonTools{dataView()}.startsWith(other.dataView(), compareFn);
}

auto U8StringCharView::endsWith(const U8StringView &other, const CharCompareFn compareFn) const noexcept -> bool {
    return impl::U8StringComparisonTools{dataView()}.endsWith(other.dataView(), compareFn);
}

auto U8StringCharView::contains(const U8StringView &other, const CharCompareFn compareFn) const noexcept -> bool {
    return impl::U8StringComparisonTools{dataView()}.contains(other.dataView(), compareFn);
}

auto U8StringCharView::containsOneOf(const CharSet &characters) const noexcept -> bool {
    return impl::U8StringComparisonTools{dataView()}.containsOneOf(characters);
}

auto U8StringCharView::containsOnly(const CharSet &characters) const noexcept -> bool {
    return impl::U8StringComparisonTools{dataView()}.containsOnly(characters);
}

}
