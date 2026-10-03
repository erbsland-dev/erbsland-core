// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16String.hpp"

#include "impl/U16StringComparisonTools.hpp"
#include "impl/U16StringReadTools.hpp"

namespace erbsland::text {

using namespace unit;

auto U16String::splitAtFirst(const U16String &separator, const CharCompareFn compareFn) const noexcept
    -> std::optional<std::pair<U16String, U16String>> {
    return splitAroundRange(impl::U16StringComparisonTools{dataView()}.findFirstRange(separator.dataView(), compareFn));
}

auto U16String::splitAtFirst(const CharSet &separators) const noexcept
    -> std::optional<std::pair<U16String, U16String>> {
    return splitAroundRange(impl::U16StringReadTools{dataView()}.findFirstRange(separators));
}

auto U16String::splitAtLast(const U16String &separator, const CharCompareFn compareFn) const noexcept
    -> std::optional<std::pair<U16String, U16String>> {
    return splitAroundRange(impl::U16StringComparisonTools{dataView()}.findLastRange(separator.dataView(), compareFn));
}

auto U16String::splitAtLast(const CharSet &separators) const noexcept
    -> std::optional<std::pair<U16String, U16String>> {
    return splitAroundRange(impl::U16StringReadTools{dataView()}.findLastRange(separators));
}

auto U16String::splitAroundRange(const U16Range match) const noexcept
    -> std::optional<std::pair<U16String, U16String>> {
    if (!match.isValid()) {
        return std::nullopt;
    }
    const auto source = dataView().range();
    const auto before = U16Range{U16Index::zero(), match.beginIndex()}.toAbsolute(source.beginIndex());
    const auto after = U16Range{match.endIndex(), U16Index::end(source.length())}.toAbsolute(source.beginIndex());
    return std::pair{withRange(before), withRange(after)};
}

}
