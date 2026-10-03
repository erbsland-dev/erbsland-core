// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32String.hpp"

#include "impl/U32StringComparisonTools.hpp"
#include "impl/U32StringReadTools.hpp"

namespace erbsland::text {

using namespace unit;

auto U32String::splitAtFirst(const U32String &separator, const CharCompareFn compareFn) const noexcept
    -> std::optional<std::pair<U32String, U32String>> {
    return splitAroundRange(impl::U32StringComparisonTools{dataView()}.findFirstRange(separator.dataView(), compareFn));
}

auto U32String::splitAtFirst(const CharSet &separators) const noexcept
    -> std::optional<std::pair<U32String, U32String>> {
    return splitAroundRange(impl::U32StringReadTools{dataView()}.findFirstRange(separators));
}

auto U32String::splitAtLast(const U32String &separator, const CharCompareFn compareFn) const noexcept
    -> std::optional<std::pair<U32String, U32String>> {
    return splitAroundRange(impl::U32StringComparisonTools{dataView()}.findLastRange(separator.dataView(), compareFn));
}

auto U32String::splitAtLast(const CharSet &separators) const noexcept
    -> std::optional<std::pair<U32String, U32String>> {
    return splitAroundRange(impl::U32StringReadTools{dataView()}.findLastRange(separators));
}

auto U32String::splitAroundRange(const CpRange match) const noexcept -> std::optional<std::pair<U32String, U32String>> {
    if (!match.isValid()) {
        return std::nullopt;
    }
    const auto source = dataView().range();
    const auto before = CpRange{CpIndex::zero(), match.beginIndex()}.toAbsolute(source.beginIndex());
    const auto after = CpRange{match.endIndex(), CpIndex::end(source.length())}.toAbsolute(source.beginIndex());
    return std::pair{withRange(before), withRange(after)};
}

}
