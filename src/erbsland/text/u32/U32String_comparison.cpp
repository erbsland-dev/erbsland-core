// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32String.hpp"

#include "U32StringEditor.hpp"

#include "impl/U32StringComparisonTools.hpp"

namespace erbsland::text {

auto U32String::compare(const U32String &other, const CharCompareFn compareFn) const noexcept -> std::strong_ordering {
    return impl::U32StringComparisonTools{dataView()}.compare(other.dataView(), compareFn);
}

auto U32String::operator<=>(const U32String &other) const noexcept -> std::strong_ordering {
    return compare(other);
}

auto U32String::startsWith(const U32String &other, const CharCompareFn compareFn) const noexcept -> bool {
    return impl::U32StringComparisonTools{dataView()}.startsWith(other.dataView(), compareFn);
}

auto U32String::endsWith(const U32String &other, const CharCompareFn compareFn) const noexcept -> bool {
    return impl::U32StringComparisonTools{dataView()}.endsWith(other.dataView(), compareFn);
}

auto U32String::contains(const U32String &other, const CharCompareFn compareFn) const noexcept -> bool {
    return impl::U32StringComparisonTools{dataView()}.contains(other.dataView(), compareFn);
}

auto U32String::count(const U32String &text, const CharCompareFn compareFn) const noexcept -> unit::ItemCount {
    return impl::U32StringComparisonTools{dataView()}.count(text.dataView(), compareFn);
}

auto U32String::containsOneOf(const CharSet &characters) const noexcept -> bool {
    return impl::U32StringComparisonTools{dataView()}.containsOneOf(characters);
}

auto U32String::containsOnly(const CharSet &characters) const noexcept -> bool {
    return impl::U32StringComparisonTools{dataView()}.containsOnly(characters);
}

auto U32String::containsOnly(const AsciiCategory category) const noexcept -> bool {
    return impl::U32StringComparisonTools{dataView()}.containsOnly(category);
}

}
