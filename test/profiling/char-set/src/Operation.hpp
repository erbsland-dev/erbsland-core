// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace app::charset {

/// One measured `CharSet` API path.
/// @notest{Covered by the character-set profiling coverage test.}
enum class Operation : std::uint8_t {
    ConstructDefault,
    ConstructChar,
    ConstructU8String,
    ConstructSet,
    ConstructList,
    ConstructInitializerList,
    CopyConstruct,
    MoveConstruct,
    CopyAssign,
    MoveAssign,
    Equal,
    NotEqual,
    SubsetOperator,
    SupersetOperator,
    IsEmpty,
    Contains,
    IsSubset,
    IsSuperset,
    IsEqualCi,
    IsSubsetCi,
    ContainsCaseFoldable,
    ContainsLowercaseMappable,
    ContainsUppercaseMappable,
    UnitedWith,
    IntersectedWith,
    SubtractedBy,
    SymmetricDifferenceWith,
    UnionOperator,
    IntersectionOperator,
    SubtractionOperator,
    SymmetricDifferenceOperator,
    UnionAssign,
    IntersectionAssign,
    SubtractionAssign,
    SymmetricDifferenceAssign,
    AddSet,
    AddRange,
    AddChar,
    RemoveSet,
    RemoveRange,
    RemoveChar,
    ForEachRange,
    ForEachChar,
    Transform,
    CaseFolded,
    ToLowercase,
    ToUppercase,
    ToString,
    ToU8String,
    ToU16String,
    ToU32String,
    ToSet,
    ToList,
    FromRange,
    FromAsciiCategory,
    FromUnicodeCategory,
    FromUnicodeGroup,
    FromPatternU8,
    FromPatternU16,
    FromPatternU32,
};

}
