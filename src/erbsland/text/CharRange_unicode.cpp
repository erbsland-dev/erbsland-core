// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CharRange.hpp"

#include "impl/UnicodeData.hpp"

namespace erbsland::text {

auto CharRange::containsCaseFoldableCharacters() const noexcept -> bool {
    return !isEmpty() && impl::unicodeContainsCaseFoldableCharacters(from().toRawValue(), to().toRawValue());
}

auto CharRange::containsLowercaseMappableCharacters() const noexcept -> bool {
    return !isEmpty() && impl::unicodeContainsLowercaseMappableCharacters(from().toRawValue(), to().toRawValue());
}

auto CharRange::containsUppercaseMappableCharacters() const noexcept -> bool {
    return !isEmpty() && impl::unicodeContainsUppercaseMappableCharacters(from().toRawValue(), to().toRawValue());
}

}
