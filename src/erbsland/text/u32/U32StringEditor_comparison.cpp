// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringEditor.hpp"

#include "U32String.hpp"

#include "impl/U32StringComparisonTools.hpp"

namespace erbsland::text {

auto U32StringEditor::compare(const U32String &other, const CharCompareFn compareFn) const noexcept
    -> std::strong_ordering {
    return impl::U32StringComparisonTools{dataView()}.compare(other.dataView(), compareFn);
}

auto U32StringEditor::operator<=>(const U32String &other) const noexcept -> std::strong_ordering {
    return compare(other);
}

auto U32StringEditor::startsWith(const U32String &other, const CharCompareFn compareFn) const noexcept -> bool {
    return impl::U32StringComparisonTools{dataView()}.startsWith(other.dataView(), compareFn);
}

auto U32StringEditor::endsWith(const U32String &other, const CharCompareFn compareFn) const noexcept -> bool {
    return impl::U32StringComparisonTools{dataView()}.endsWith(other.dataView(), compareFn);
}

auto U32StringEditor::contains(const U32String &other, const CharCompareFn compareFn) const noexcept -> bool {
    return impl::U32StringComparisonTools{dataView()}.contains(other.dataView(), compareFn);
}

auto U32StringEditor::count(const U32String &text, const CharCompareFn compareFn) const noexcept -> unit::ElementCount {
    return impl::U32StringComparisonTools{dataView()}.count(text.dataView(), compareFn);
}

auto U32StringEditor::containsOneOf(const CharSet &characters) const noexcept -> bool {
    return impl::U32StringComparisonTools{dataView()}.containsOneOf(characters);
}

auto U32StringEditor::containsOnly(const CharSet &characters) const noexcept -> bool {
    return impl::U32StringComparisonTools{dataView()}.containsOnly(characters);
}

}
