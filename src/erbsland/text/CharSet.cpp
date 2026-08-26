// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CharSet.hpp"

#include "impl/CharSetFromPattern.hpp"
#include "impl/CharSetRangeBuilder.hpp"
#include "impl/CharSetRangeCounter.hpp"
#include "impl/ThrowHelper.hpp"
#include "u16/U16String.hpp"
#include "u16/U16StringEditor.hpp"
#include "u32/U32String.hpp"
#include "u32/U32StringEditor.hpp"
#include "u8/U8String.hpp"
#include "u8/U8StringEditor.hpp"

#include "../mem/impl/SharedArrayCapacity.hpp"
#include "../mem/SharedArrayData.hpp"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <limits>

namespace erbsland::text {

CharSet::CharSet(const Char character) {
    add(character);
}

CharSet::CharSet(const U8String &characters) {
    const auto capacityHint = characters.length().toSizeT();
    characters.forEach([this, capacityHint](const Char character) -> util::LoopStatus {
        addWithCapacity(CharRange{character}, capacityHint);
        return util::LoopStatus::Continue;
    });
}

CharSet::CharSet(const util::Set<Char> &characters) {
    auto builder = impl::CharSetRangeBuilder{characters.count().toSizeT()};
    for (const auto character : characters) {
        builder.add(CharRange{character});
    }
    *this = builder.take();
}

CharSet::CharSet(const util::List<Char> &characters) {
    const auto capacityHint = characters.count().toSizeT();
    for (const auto character : characters) {
        addWithCapacity(CharRange{character}, capacityHint);
    }
}

CharSet::CharSet(const std::initializer_list<Char> characters) {
    for (const auto character : characters) {
        addWithCapacity(CharRange{character}, characters.size());
    }
}

CharSet::~CharSet() = default;

CharSet::CharSet(const CharSet &) noexcept = default;

CharSet::CharSet(CharSet &&other) noexcept : _storage{other._storage} {
}

auto CharSet::operator=(const CharSet &) noexcept -> CharSet & = default;

auto CharSet::operator=(CharSet &&other) noexcept -> CharSet & {
    if (this != &other) {
        _storage = other._storage;
    }
    return *this;
}

auto CharSet::operator==(const CharSet &other) const noexcept -> bool {
    const auto left = rangeSpan();
    const auto right = other.rangeSpan();
    return std::ranges::equal(left, right);
}

auto CharSet::operator|=(const CharSet &other) -> CharSet & {
    add(other);
    return *this;
}

auto CharSet::operator&=(const CharSet &other) -> CharSet & {
    *this = intersectedWith(other);
    return *this;
}

auto CharSet::operator-=(const CharSet &other) -> CharSet & {
    remove(other);
    return *this;
}

auto CharSet::operator^=(const CharSet &other) -> CharSet & {
    *this = symmetricDifferenceWith(other);
    return *this;
}

auto CharSet::contains(const Char character) const noexcept -> bool {
    if (!character.isValidUnicode()) {
        return false;
    }
    const auto ranges = rangeSpan();
    if (ranges.size() <= 2U) {
        return std::ranges::any_of(
            ranges, [character](const CharRange range) noexcept -> bool { return range.contains(character); });
    }
    const auto it = std::ranges::upper_bound(
        ranges, character, {}, [](const CharRange range) noexcept -> Char { return range.from(); });
    return it != ranges.begin() && std::prev(it)->contains(character);
}

auto CharSet::isSubsetOf(const CharSet &other) const -> bool {
    const auto left = rangeSpan();
    const auto right = other.rangeSpan();
    auto rightIndex = std::size_t{0};
    for (const auto leftRange : left) {
        while (rightIndex < right.size() && right[rightIndex].to() < leftRange.from()) {
            ++rightIndex;
        }
        if (rightIndex == right.size() || right[rightIndex].from() > leftRange.from() ||
            right[rightIndex].to() < leftRange.to()) {
            return false;
        }
    }
    return true;
}

void CharSet::add(const CharSet &other) {
    if (other.isEmpty()) {
        return;
    }
    *this = unitedWith(other);
}

void CharSet::add(CharRange range) {
    addWithCapacity(range, 0U);
}

void CharSet::addWithCapacity(CharRange range, const std::size_t capacityHint) {
    if (range.isEmpty()) {
        return;
    }

    const auto oldRanges = rangeSpan();
    const auto insertion = std::ranges::lower_bound(
        oldRanges, range.from(), {}, [](const CharRange existing) noexcept -> Char { return existing.from(); });
    auto first = static_cast<std::size_t>(std::distance(oldRanges.begin(), insertion));
    if (first > 0U && oldRanges[first - 1U].canMergeWith(range)) {
        --first;
    }
    auto after = first;
    while (after < oldRanges.size() && range.canMergeWith(oldRanges[after])) {
        range = range.mergedWith(oldRanges[after]);
        ++after;
    }
    if (first < after && after - first == 1U && range == oldRanges[first]) {
        return;
    }

    const auto removedCount = after - first;
    const auto newCount = oldRanges.size() - removedCount + 1U;
    if (newCount <= 2U) {
        auto replacement = InlineRanges{};
        auto destination = std::size_t{0};
        for (auto index = std::size_t{0}; index < first; ++index) {
            replacement.values[destination++] = oldRanges[index];
        }
        replacement.values[destination++] = range;
        for (auto index = after; index < oldRanges.size(); ++index) {
            replacement.values[destination++] = oldRanges[index];
        }
        _storage = replacement;
        return;
    }

    ensureSharedCapacity(std::max(newCount, capacityHint));
    auto &sharedRanges = std::get<RangeDataPtr>(_storage);
    auto *data = sharedRanges.get();
    auto *values = data->data();
    if (after != first + 1U && after < static_cast<std::size_t>(data->size())) {
        std::memmove(
            values + first + 1U, values + after, (static_cast<std::size_t>(data->size()) - after) * sizeof(CharRange));
    }
    values[first] = range;
    data->setSize(static_cast<RangeData::SizeType>(newCount));
}

void CharSet::add(const Char character) {
    add(CharRange{character});
}

void CharSet::remove(const CharSet &other) {
    if (isEmpty() || other.isEmpty()) {
        return;
    }
    *this = subtractedBy(other);
}

void CharSet::remove(const CharRange range) {
    if (isEmpty() || range.isEmpty()) {
        return;
    }
    const auto source = rangeSpan();
    auto builder = impl::CharSetRangeBuilder{source.size() + 1U};
    for (const auto existing : source) {
        if (!existing.overlaps(range)) {
            builder.add(existing);
            continue;
        }
        if (existing.from() < range.from()) {
            if (const auto leftEnd = previousScalar(range.from()); leftEnd.has_value() && existing.from() <= *leftEnd) {
                builder.add(CharRange{existing.from(), *leftEnd});
            }
        }
        if (existing.to() > range.to()) {
            if (const auto rightStart = nextScalar(range.to());
                rightStart.has_value() && *rightStart <= existing.to()) {
                builder.add(CharRange{*rightStart, existing.to()});
            }
        }
    }
    *this = builder.take();
}

void CharSet::remove(const Char character) {
    remove(CharRange{character});
}

auto CharSet::unitedWith(const CharSet &other) const -> CharSet {
    const auto left = rangeSpan();
    const auto right = other.rangeSpan();
    auto builder = impl::CharSetRangeBuilder{left.size() + right.size()};
    auto leftIndex = std::size_t{0};
    auto rightIndex = std::size_t{0};
    while (leftIndex < left.size() || rightIndex < right.size()) {
        if (rightIndex == right.size() ||
            (leftIndex < left.size() && left[leftIndex].from() <= right[rightIndex].from())) {
            builder.add(left[leftIndex++]);
        } else {
            builder.add(right[rightIndex++]);
        }
    }
    return builder.take();
}

auto CharSet::intersectedWith(const CharSet &other) const -> CharSet {
    const auto left = rangeSpan();
    const auto right = other.rangeSpan();
    auto builder = impl::CharSetRangeBuilder{left.size() + right.size()};
    auto leftIndex = std::size_t{0};
    auto rightIndex = std::size_t{0};
    while (leftIndex < left.size() && rightIndex < right.size()) {
        const auto leftRange = left[leftIndex];
        const auto rightRange = right[rightIndex];
        if (leftRange.to() < rightRange.from()) {
            ++leftIndex;
            continue;
        }
        if (rightRange.to() < leftRange.from()) {
            ++rightIndex;
            continue;
        }
        builder.add(
            CharRange{std::max(leftRange.from(), rightRange.from()), std::min(leftRange.to(), rightRange.to())});
        if (leftRange.to() < rightRange.to()) {
            ++leftIndex;
        } else {
            ++rightIndex;
        }
    }
    return builder.take();
}

auto CharSet::subtractedBy(const CharSet &other) const -> CharSet {
    const auto left = rangeSpan();
    const auto right = other.rangeSpan();
    auto builder = impl::CharSetRangeBuilder{left.size() + right.size()};
    auto rightIndex = std::size_t{0};
    for (const auto leftRange : left) {
        auto remainderStart = leftRange.from();
        while (rightIndex < right.size() && right[rightIndex].to() < remainderStart) {
            ++rightIndex;
        }
        auto scanIndex = rightIndex;
        auto consumed = false;
        while (scanIndex < right.size() && right[scanIndex].from() <= leftRange.to()) {
            const auto rightRange = right[scanIndex];
            if (rightRange.from() > remainderStart) {
                const auto end = previousScalar(rightRange.from());
                if (end.has_value() && remainderStart <= *end) {
                    builder.add(CharRange{remainderStart, std::min(*end, leftRange.to())});
                }
            }
            if (rightRange.to() >= leftRange.to()) {
                consumed = true;
                break;
            }
            const auto next = nextScalar(rightRange.to());
            if (!next.has_value() || *next > leftRange.to()) {
                consumed = true;
                break;
            }
            remainderStart = *next;
            ++scanIndex;
        }
        if (!consumed && remainderStart <= leftRange.to()) {
            builder.add(CharRange{remainderStart, leftRange.to()});
        }
    }
    return builder.take();
}

}
