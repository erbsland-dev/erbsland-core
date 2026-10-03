// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8String.hpp"

#include "impl/U8StringComparisonTools.hpp"
#include "impl/U8StringReadTools.hpp"

namespace erbsland::text {

using namespace unit;

auto U8String::splitAtFirst(const U8String &separator, const CharCompareFn compareFn) const noexcept
    -> std::optional<std::pair<U8String, U8String>> {
    return splitAroundRange(impl::U8StringComparisonTools{dataView()}.findFirstRange(separator.dataView(), compareFn));
}

auto U8String::splitAtFirst(const CharSet &separators) const noexcept -> std::optional<std::pair<U8String, U8String>> {
    return splitAroundRange(impl::U8StringReadTools{dataView()}.findFirstRange(separators));
}

auto U8String::splitAtLast(const U8String &separator, const CharCompareFn compareFn) const noexcept
    -> std::optional<std::pair<U8String, U8String>> {
    return splitAroundRange(impl::U8StringComparisonTools{dataView()}.findLastRange(separator.dataView(), compareFn));
}

auto U8String::splitAtLast(const CharSet &separators) const noexcept -> std::optional<std::pair<U8String, U8String>> {
    return splitAroundRange(impl::U8StringReadTools{dataView()}.findLastRange(separators));
}

auto U8String::splitAroundRange(const ByteRange match) const noexcept -> std::optional<std::pair<U8String, U8String>> {
    if (!match.isValid()) {
        return std::nullopt;
    }
    const auto source = dataView().range();
    const auto before = ByteRange{ByteIndex::zero(), match.beginIndex()}.toAbsolute(source.beginIndex());
    const auto after = ByteRange{match.endIndex(), ByteIndex::end(source.length())}.toAbsolute(source.beginIndex());
    return std::pair{withRange(before), withRange(after)};
}

}
