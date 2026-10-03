// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringEditor.hpp"

#include "U8String.hpp"

#include "impl/U8StringComparisonTools.hpp"
#include "impl/U8StringReadTools.hpp"

namespace erbsland::text {

using namespace unit;

auto U8StringEditor::splitAtFirst(const U8String &separator, const CharCompareFn compareFn) const noexcept
    -> std::optional<std::pair<U8StringEditor, U8StringEditor>> {
    return splitAroundRange(impl::U8StringComparisonTools{dataView()}.findFirstRange(separator.dataView(), compareFn));
}

auto U8StringEditor::splitAtFirst(const CharSet &separators) const noexcept
    -> std::optional<std::pair<U8StringEditor, U8StringEditor>> {
    return splitAroundRange(impl::U8StringReadTools{dataView()}.findFirstRange(separators));
}

auto U8StringEditor::splitAtLast(const U8String &separator, const CharCompareFn compareFn) const noexcept
    -> std::optional<std::pair<U8StringEditor, U8StringEditor>> {
    return splitAroundRange(impl::U8StringComparisonTools{dataView()}.findLastRange(separator.dataView(), compareFn));
}

auto U8StringEditor::splitAtLast(const CharSet &separators) const noexcept
    -> std::optional<std::pair<U8StringEditor, U8StringEditor>> {
    return splitAroundRange(impl::U8StringReadTools{dataView()}.findLastRange(separators));
}

auto U8StringEditor::splitAroundRange(const ByteRange match) const noexcept
    -> std::optional<std::pair<U8StringEditor, U8StringEditor>> {
    if (!match.isValid()) {
        return std::nullopt;
    }
    const auto source = dataView().range();
    const auto before = ByteRange{ByteIndex::zero(), match.beginIndex()}.toAbsolute(source.beginIndex());
    const auto after = ByteRange{match.endIndex(), ByteIndex::end(source.length())}.toAbsolute(source.beginIndex());
    return std::pair{withRange(before), withRange(after)};
}

}
