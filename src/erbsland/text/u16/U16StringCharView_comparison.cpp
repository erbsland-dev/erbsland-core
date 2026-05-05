// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringCharView.hpp"

#include "U16StringView.hpp"

#include "impl/U16StringComparisonTools.hpp"

namespace erbsland::text {

auto U16StringCharView::isEmpty() const noexcept -> bool {
    return length().isZero();
}

auto U16StringCharView::startsWith(const U16StringView &other, const CharCompareFn compareFn) const noexcept -> bool {
    return impl::U16StringComparisonTools{dataView()}.startsWith(other.dataView(), compareFn);
}

auto U16StringCharView::endsWith(const U16StringView &other, const CharCompareFn compareFn) const noexcept -> bool {
    return impl::U16StringComparisonTools{dataView()}.endsWith(other.dataView(), compareFn);
}

auto U16StringCharView::contains(const U16StringView &other, const CharCompareFn compareFn) const noexcept -> bool {
    return impl::U16StringComparisonTools{dataView()}.contains(other.dataView(), compareFn);
}

auto U16StringCharView::containsOneOf(const CharSet &characters) const noexcept -> bool {
    return impl::U16StringComparisonTools{dataView()}.containsOneOf(characters);
}

auto U16StringCharView::containsOnly(const CharSet &characters) const noexcept -> bool {
    return impl::U16StringComparisonTools{dataView()}.containsOnly(characters);
}

}
