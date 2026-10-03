// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringTransformationSeparators.hpp"

#include "../CharRange.hpp"
#include "../CharSet.hpp"

namespace erbsland::text::impl {

auto asciiTransformationSeparators() -> const CharSet & {
    static const auto cSeparators = CharSet{CharRange{U'\t', U'\r'}, CharRange{U' '}};
    return cSeparators;
}

}
