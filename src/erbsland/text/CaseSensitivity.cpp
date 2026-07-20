// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CaseSensitivity.hpp"

#include "Char.hpp"
#include "String.hpp"

namespace erbsland::text {

using namespace literals;

auto CaseSensitivity::comparisonFn() const noexcept -> CharCompareFn {
    return _value == CaseInsensitive ? static_cast<CharCompareFn>(&Char::compareCaseFolded) : nullptr;
}

auto CaseSensitivity::asciiComparisonFn() const noexcept -> CharCompareFn {
    return _value == CaseInsensitive ? static_cast<CharCompareFn>(&Char::compareAsciiFolded) : nullptr;
}

auto CaseSensitivity::toString() const noexcept -> String {
    switch (_value) {
    case CaseSensitive:
        return "case-sensitive"_el;
    case CaseInsensitive:
        return "case-insensitive"_el;
    }
    return {};
}

}
