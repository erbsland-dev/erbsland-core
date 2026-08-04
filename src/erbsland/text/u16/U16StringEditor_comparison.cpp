// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringEditor.hpp"

#include "U16String.hpp"

#include "impl/U16StringComparisonTools.hpp"

namespace erbsland::text {

auto U16StringEditor::compare(const U16String &other, const CharCompareFn compareFn) const noexcept
    -> std::strong_ordering {
    return impl::U16StringComparisonTools{dataView()}.compare(other.dataView(), compareFn);
}

auto U16StringEditor::operator<=>(const U16String &other) const noexcept -> std::strong_ordering {
    return compare(other);
}

auto U16StringEditor::startsWith(const U16String &other, const CharCompareFn compareFn) const noexcept -> bool {
    return impl::U16StringComparisonTools{dataView()}.startsWith(other.dataView(), compareFn);
}

auto U16StringEditor::endsWith(const U16String &other, const CharCompareFn compareFn) const noexcept -> bool {
    return impl::U16StringComparisonTools{dataView()}.endsWith(other.dataView(), compareFn);
}

auto U16StringEditor::contains(const U16String &other, const CharCompareFn compareFn) const noexcept -> bool {
    return impl::U16StringComparisonTools{dataView()}.contains(other.dataView(), compareFn);
}

auto U16StringEditor::count(const U16String &text, const CharCompareFn compareFn) const noexcept -> unit::ItemCount {
    return impl::U16StringComparisonTools{dataView()}.count(text.dataView(), compareFn);
}

auto U16StringEditor::containsOneOf(const CharSet &characters) const noexcept -> bool {
    return impl::U16StringComparisonTools{dataView()}.containsOneOf(characters);
}

auto U16StringEditor::containsOnly(const CharSet &characters) const noexcept -> bool {
    return impl::U16StringComparisonTools{dataView()}.containsOnly(characters);
}

}
