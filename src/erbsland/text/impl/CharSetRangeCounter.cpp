// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CharSetRangeCounter.hpp"

#include <cassert>

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

}
