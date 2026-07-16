// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CharRange.hpp"

#include "../error/InternalError.hpp"

#include <algorithm>
#include <ranges>

namespace erbsland::re::impl {

auto CharRange::matches(const text::Char character) const noexcept -> bool {
    return character >= _first && character <= _last;
}

auto CharRange::toString() const -> text::String {
    text::String result;
    if (_first == _last) {
        appendToCharRangeString(result, _first);
    } else {
        appendToCharRangeString(result, _first);
        result.append(U'-');
        appendToCharRangeString(result, _last);
    }
    return result;
}

}
