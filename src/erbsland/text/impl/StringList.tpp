// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringSplitter.hpp"

#include "../AnyStringBuilder.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <utility>

namespace erbsland::text::impl {

template <typename tString>
auto StringList<tString>::compare(const StringList &other, const CharCompareFn compareFn) const noexcept
    -> std::strong_ordering {
    const auto &left = this->raw();
    const auto &right = other.raw();
    const auto sharedSize = std::min(left.size(), right.size());
    for (auto index = std::size_t{0}; index < sharedSize; ++index) {
        const auto order = left[index].compare(right[index], compareFn);
        if (order != std::strong_ordering::equal) {
            return order;
        }
    }
    if (left.size() < right.size()) {
        return std::strong_ordering::less;
    }
    if (left.size() > right.size()) {
        return std::strong_ordering::greater;
    }
    return std::strong_ordering::equal;
}

template <typename tString>
auto StringList<tString>::sort(const CharCompareFn compareFn) -> StringList & {
    auto &data = this->mutableRaw();
    std::sort(data.begin(), data.end(), [compareFn](const Element &left, const Element &right) -> bool {
        return left.compare(right, compareFn) < 0;
    });
    return *this;
}

template <typename tString>
auto StringList<tString>::sort() -> StringList & {
    return sort(static_cast<CharCompareFn>(nullptr));
}

template <typename tString>
auto StringList<tString>::sorted(const CharCompareFn compareFn) const -> StringList {
    auto result = *this;
    result.sort(compareFn);
    return result;
}

template <typename tString>
auto StringList<tString>::sorted() const -> StringList {
    auto result = *this;
    result.sort();
    return result;
}

template <typename tString>
auto StringList<tString>::findFirst(const Element &value, const CharCompareFn compareFn) const noexcept -> Index {
    return findFirst(value, Index::zero(), compareFn);
}

template <typename tString>
auto StringList<tString>::findFirst(
    const Element &value, const Index start, const CharCompareFn compareFn) const noexcept -> Index {
    return this->findFirstIf(
        [&](const Element &entry) -> bool { return entry.compare(value, compareFn) == std::strong_ordering::equal; },
        start);
}

template <typename tString>
auto StringList<tString>::findLast(const Element &value, const CharCompareFn compareFn) const noexcept -> Index {
    return this->findLastIf(
        [&](const Element &entry) -> bool { return entry.compare(value, compareFn) == std::strong_ordering::equal; });
}

template <typename tString>
auto StringList<tString>::findLast(
    const Element &value, const Index start, const CharCompareFn compareFn) const noexcept -> Index {
    return this->findLastIf(
        [&](const Element &entry) -> bool { return entry.compare(value, compareFn) == std::strong_ordering::equal; },
        start);
}

template <typename tString>
auto StringList<tString>::contains(const Element &value, const CharCompareFn compareFn) const noexcept -> bool {
    return !findFirst(value, compareFn).isNoIndex();
}

template <typename tString>
auto StringList<tString>::fromSplit(
    const ReadOnly &text, const CharSet &separators, const Count maximumSplits, const bool keepEmpty) -> StringList {
    auto result = typename Base::Raw{};
    result.reserve(estimatedSplitCapacity(text, separators, maximumSplits));

    if (maximumSplits.isZero()) {
        result.emplace_back(text);
        return StringList{std::move(result)};
    }
    if (text.isEmpty()) {
        if (keepEmpty) {
            result.emplace_back(text);
        }
        return StringList{std::move(result)};
    }
    if (separators.isEmpty()) {
        result.emplace_back(text);
        return StringList{std::move(result)};
    }

    auto splitter = StringSplitter<ReadOnly>{text, separators};
    auto splitCount = Count::zero();
    while (!splitter.isAtEnd() && (maximumSplits.isInfinite() || splitCount < maximumSplits)) {
        auto part = splitter.next();
        if (keepEmpty || !part.isEmpty()) {
            result.emplace_back(std::move(part));
        }
        ++splitCount;
    }
    if (!splitter.isAtEnd()) {
        auto part = splitter.remaining();
        if (keepEmpty || !part.isEmpty()) {
            result.emplace_back(std::move(part));
        }
    }
    return StringList{std::move(result)};
}

template <typename tString>
auto StringList<tString>::join(const ReadOnly &separator) const -> Element {
    const auto &raw = this->raw();
    if (raw.empty()) {
        return Element{};
    }

    auto finalLength = NativeLength{};
    auto first = true;
    for (const auto &part : raw) {
        if (!first) {
            finalLength = finalLength.addedOrThrow(separator.length());
        }
        finalLength = finalLength.addedOrThrow(ReadOnly{part}.length());
        first = false;
    }

    auto result = AnyStringBuilder::basedOn(ReadOnly{}, finalLength);
    first = true;
    for (const auto &part : raw) {
        if (!first) {
            result.append(separator);
        }
        result.append(ReadOnly{part});
        first = false;
    }
    return Element{result.template toEditor<typename StringTypes<Element>::Editor>()};
}

template <typename tString>
auto StringList<tString>::removeEmpty() -> StringList & {
    return this->removeIf([](const Element &value) -> bool { return value.isEmpty(); });
}

template <typename tString>
auto StringList<tString>::removedEmpty() const -> StringList {
    auto result = *this;
    result.removeEmpty();
    return result;
}

template <typename tString>
auto StringList<tString>::estimatedSplitCapacity(
    const ReadOnly &text, const CharSet &separators, const Count maximumSplits) -> std::size_t {
    constexpr auto cSmallSplitLimit = std::size_t{32U};
    constexpr auto cSampleLength = std::size_t{1024U};
    constexpr auto cMaximumReservation = std::size_t{100'000U};

    if (maximumSplits.isZero() || text.isEmpty() || separators.isEmpty()) {
        return 1U;
    }
    if (maximumSplits.isFinite() && maximumSplits < Count{cSmallSplitLimit}) {
        return capacityForSplitLimit(maximumSplits.toSizeTOrThrow());
    }

    const auto textLength = text.length().toSizeT();
    const auto sampleLength = std::min(textLength, cSampleLength);
    const auto sampleEnd = NativeIndex::end(NativeLength::fromSizeT(sampleLength));
    const auto finiteMaximumSplits =
        maximumSplits.isFinite() ? maximumSplits.toSizeTOrThrow() : std::numeric_limits<std::size_t>::max();

    auto sampledSplits = std::size_t{0};
    auto searchPosition = NativeIndex::zero();
    while (searchPosition < sampleEnd && sampledSplits < finiteMaximumSplits) {
        const auto splitPosition = text.findFirstOf(separators, searchPosition);
        if (splitPosition.isNoIndex() || splitPosition >= sampleEnd) {
            break;
        }
        ++sampledSplits;
        searchPosition = splitPosition;
        text.advance(searchPosition);
    }
    if (sampledSplits == finiteMaximumSplits) {
        return capacityForSplitLimit(finiteMaximumSplits);
    }
    if (sampledSplits == 0U) {
        return 1U;
    }

    const auto estimatedSplitRatio = static_cast<long double>(sampledSplits) * static_cast<long double>(textLength) /
        static_cast<long double>(sampleLength);
    const auto estimatedSplits = static_cast<std::size_t>(std::ceil(estimatedSplitRatio));
    auto estimatedParts = estimatedSplits >= cMaximumReservation ? cMaximumReservation : estimatedSplits + 1U;
    const auto growth = estimatedParts / 20U;
    estimatedParts = growth > cMaximumReservation - estimatedParts ? cMaximumReservation : estimatedParts + growth;
    estimatedParts = std::min(estimatedParts, cMaximumReservation);
    if (maximumSplits.isFinite()) {
        estimatedParts = std::min(estimatedParts, capacityForSplitLimit(finiteMaximumSplits));
    }
    return std::max(estimatedParts, std::size_t{1U});
}

template <typename tString>
auto StringList<tString>::capacityForSplitLimit(const std::size_t maximumSplits) noexcept -> std::size_t {
    if (maximumSplits == std::numeric_limits<std::size_t>::max()) {
        return maximumSplits;
    }
    return maximumSplits + 1U;
}

}
