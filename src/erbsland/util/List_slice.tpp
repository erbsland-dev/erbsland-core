// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::util {

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::sliceFirst() const -> std::pair<Element, Self> {
    if (raw().empty()) {
        return {Element{}, Self{}};
    }
    return {raw().front(), slice(Range{Index::one(), Count::fromSizeT(raw().size() - 1U)})};
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::sliceLast() const -> std::pair<Element, Self> {
    if (raw().empty()) {
        return {Element{}, Self{}};
    }
    return {raw().back(), prefix(Count::fromSizeT(raw().size() - 1U))};
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::slice(const Range range) const -> Self {
    const auto &data = raw();
    const auto clampedRange = range.clampedTo(count());
    if (!clampedRange.isValid() || clampedRange.isEmpty()) {
        return {};
    }
    const auto begin = clampedRange.index().toSizeT();
    const auto end = clampedRange.endIndex().toSizeT();
    return makeSelf(
        Raw{data.begin() + static_cast<typename Raw::difference_type>(begin),
            data.begin() + static_cast<typename Raw::difference_type>(end)});
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::prefix(const Count count) const -> Self {
    return slice(Range{Index::zero(), count});
}

template <typename tElement, typename tSelf>
auto List<tElement, tSelf>::suffix(const Count count) const -> Self {
    const auto effectiveCount = count > this->count() ? this->count() : count;
    const auto start = this->count().subtracted(effectiveCount);
    return slice(Range{Index::end(start), effectiveCount});
}

}
