// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16String.hpp"

#include "U16StringEditor.hpp"

#include "impl/U16StringComparisonTools.hpp"

namespace erbsland::text {

auto U16String::compare(const U16String &other, const CharCompareFn compareFn) const noexcept -> std::strong_ordering {
    return impl::U16StringComparisonTools{dataView()}.compare(other.dataView(), compareFn);
}

auto U16String::operator<=>(const U16String &other) const noexcept -> std::strong_ordering {
    return compare(other);
}

auto U16String::startsWith(const U16String &other, const CharCompareFn compareFn) const noexcept -> bool {
    return impl::U16StringComparisonTools{dataView()}.startsWith(other.dataView(), compareFn);
}

auto U16String::endsWith(const U16String &other, const CharCompareFn compareFn) const noexcept -> bool {
    return impl::U16StringComparisonTools{dataView()}.endsWith(other.dataView(), compareFn);
}

auto U16String::contains(const U16String &other, const CharCompareFn compareFn) const noexcept -> bool {
    return impl::U16StringComparisonTools{dataView()}.contains(other.dataView(), compareFn);
}

auto U16String::count(const U16String &text, const CharCompareFn compareFn) const noexcept -> unit::ItemCount {
    return impl::U16StringComparisonTools{dataView()}.count(text.dataView(), compareFn);
}

auto U16String::containsOneOf(const CharSet &characters) const noexcept -> bool {
    return impl::U16StringComparisonTools{dataView()}.containsOneOf(characters);
}

auto U16String::containsOnly(const CharSet &characters) const noexcept -> bool {
    return impl::U16StringComparisonTools{dataView()}.containsOnly(characters);
}

}
