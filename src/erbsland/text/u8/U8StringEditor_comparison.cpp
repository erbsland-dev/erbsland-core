// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringEditor.hpp"

#include "U8String.hpp"

#include "impl/U8StringComparisonTools.hpp"

namespace erbsland::text {

auto U8StringEditor::compare(const U8String &other, const CharCompareFn compareFn) const noexcept
    -> std::strong_ordering {
    return impl::U8StringComparisonTools{dataView()}.compare(other.dataView(), compareFn);
}

auto U8StringEditor::operator<=>(const U8String &other) const noexcept -> std::strong_ordering {
    return compare(other);
}

auto U8StringEditor::startsWith(const U8String &other, const CharCompareFn compareFn) const noexcept -> bool {
    return impl::U8StringComparisonTools{dataView()}.startsWith(other.dataView(), compareFn);
}

auto U8StringEditor::endsWith(const U8String &other, const CharCompareFn compareFn) const noexcept -> bool {
    return impl::U8StringComparisonTools{dataView()}.endsWith(other.dataView(), compareFn);
}

auto U8StringEditor::contains(const U8String &other, const CharCompareFn compareFn) const noexcept -> bool {
    return impl::U8StringComparisonTools{dataView()}.contains(other.dataView(), compareFn);
}

auto U8StringEditor::count(const U8String &text, const CharCompareFn compareFn) const noexcept -> unit::ItemCount {
    return impl::U8StringComparisonTools{dataView()}.count(text.dataView(), compareFn);
}

auto U8StringEditor::containsOneOf(const CharSet &characters) const noexcept -> bool {
    return impl::U8StringComparisonTools{dataView()}.containsOneOf(characters);
}

auto U8StringEditor::containsOnly(const CharSet &characters) const noexcept -> bool {
    return impl::U8StringComparisonTools{dataView()}.containsOnly(characters);
}

}
