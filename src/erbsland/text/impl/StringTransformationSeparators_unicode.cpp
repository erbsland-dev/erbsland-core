// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringTransformationSeparators.hpp"

#include "../Char.hpp"
#include "../CharSet.hpp"
#include "../UnicodeCategoryGroup.hpp"

namespace erbsland::text::impl {

auto unicodeTransformationSeparators() -> const CharSet & {
    static const auto cSeparators = []() -> CharSet {
        auto result = asciiTransformationSeparators();
        result.add(Char{0x0085U});
        result.add(CharSet::from(UnicodeCategoryGroup::Separator));
        return result;
    }();
    return cSeparators;
}

}
