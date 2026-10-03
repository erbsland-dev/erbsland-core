// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16StringData_fwd.hpp"
#include "U16StringSharedStorage_fwd.hpp"

#include "../../../unit/CpCount.hpp"
#include "../../../unit/CpIndex.hpp"
#include "../../../unit/CpRange.hpp"
#include "../../../unit/U16Count.hpp"
#include "../../../unit/U16Index.hpp"
#include "../../../unit/U16Range.hpp"
#include "../../StringKind.hpp"

#include <cstddef>
#include <string_view>

namespace erbsland::text::impl {

/// Type bindings for UTF-16 string implementations.
/// @tested{U16StringDataTest}
struct U16StringTraits final {
    using CodeUnit = char16_t;
    using SharedStorage = U16StringSharedStorage;
    using StorageData = U16StringData;
    using StoragePtr = U16StringDataPtr;
    using StandardView = std::u16string_view;
    using LiteralChar = char16_t;
    using DataIndex = unit::U16Index;
    using DataLength = unit::U16Count;
    using DataRange = unit::U16Range;
    using CharacterIndex = unit::CpIndex;
    using CharacterLength = unit::CpCount;
    using CharacterRange = unit::CpRange;

    static constexpr auto kind = StringKind::U16; ///< String encoding represented by these traits.
    static constexpr CodeUnit terminator = u'\0';
    static constexpr auto codeUnitSize = sizeof(CodeUnit);
};

}
