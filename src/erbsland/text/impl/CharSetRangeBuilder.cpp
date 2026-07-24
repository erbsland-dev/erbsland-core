// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CharSetRangeBuilder.hpp"

#include "ThrowHelper.hpp"

#include "../../mem/SharedArrayData.hpp"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <span>

namespace erbsland::text::impl {

void CharSetRangeCounter::add(const CharRange range) noexcept {
    if (range.isEmpty()) {
        return;
    }
    if (_lastRange.isEmpty()) {
        _lastRange = range;
        _count = 1U;
        return;
    }
    assert(_lastRange.from() <= range.from());
    if (_lastRange.canMergeWith(range)) {
        _lastRange = _lastRange.mergedWith(range);
        return;
    }
    _lastRange = range;
    ++_count;
}

CharSetRangeBuilder::CharSetRangeBuilder(const std::size_t maximumRangeCount) noexcept :
    _maximumRangeCount{maximumRangeCount} {
}

CharSetRangeBuilder::~CharSetRangeBuilder() = default;

void CharSetRangeBuilder::add(CharRange range) {
    if (range.isEmpty()) {
        return;
    }
    if (count() == 0U) {
        append(range);
        return;
    }
    assert(last().from() <= range.from());
    if (last().canMergeWith(range)) {
        last() = last().mergedWith(range);
        return;
    }
    append(range);
}

auto CharSetRangeBuilder::take() -> CharSet {
    auto result = CharSet{};
    if (_sharedRanges.isNull()) {
        result._storage = _inlineRanges;
    } else {
        result._storage = std::move(_sharedRanges);
    }
    return result;
}

auto CharSetRangeBuilder::count() const noexcept -> std::size_t {
    if (!_sharedRanges.isNull()) {
        return static_cast<std::size_t>(_sharedRanges.constGet()->size());
    }
    return CharSet::inlineRangeCount(_inlineRanges);
}

auto CharSetRangeBuilder::last() noexcept -> CharRange & {
    if (!_sharedRanges.isNull()) {
        return _sharedRanges.get()->data()[_sharedRanges.constGet()->size() - 1U];
    }
    return _inlineRanges.values[CharSet::inlineRangeCount(_inlineRanges) - 1U];
}

void CharSetRangeBuilder::append(const CharRange range) {
    const auto oldCount = count();
    if (_sharedRanges.isNull() && oldCount < _inlineRanges.values.size()) {
        _inlineRanges.values[oldCount] = range;
        return;
    }
    if (_sharedRanges.isNull()) {
        promote();
    }
    auto *data = _sharedRanges.get();
    assert(static_cast<std::size_t>(data->size()) < static_cast<std::size_t>(data->capacity()));
    data->data()[data->size()] = range;
    data->setSize(static_cast<CharSet::RangeData::SizeType>(data->size() + 1U));
}

void CharSetRangeBuilder::promote() {
    const auto capacity = std::max(_maximumRangeCount, std::size_t{3U});
    if (!CharSet::RangeData::canAllocateWithCapacity(capacity)) {
        throwOverflow("Character set range count exceeds supported bounds");
    }
    constexpr auto inlineCount = std::size_t{2U};
    auto replacement = CharSet::RangeDataPtr{CharSet::RangeData::create(
        static_cast<CharSet::RangeData::SizeType>(inlineCount), static_cast<CharSet::RangeData::SizeType>(capacity))};
    std::memcpy(replacement.get()->data(), _inlineRanges.values.data(), inlineCount * sizeof(CharRange));
    _sharedRanges = std::move(replacement);
}

}
