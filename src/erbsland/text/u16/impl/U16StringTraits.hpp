// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16StringData_fwd.hpp"
#include "U16StringSharedStorage_fwd.hpp"

#include "../../../unit/CpIndex.hpp"
#include "../../../unit/CpLength.hpp"
#include "../../../unit/CpRange.hpp"
#include "../../../unit/U16DataIndex.hpp"
#include "../../../unit/U16DataLength.hpp"
#include "../../../unit/U16DataRange.hpp"

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
    using DataIndex = unit::U16DataIndex;
    using DataLength = unit::U16DataLength;
    using DataRange = unit::U16DataRange;
    using CharacterIndex = unit::CpIndex;
    using CharacterLength = unit::CpLength;
    using CharacterRange = unit::CpRange;

    static constexpr CodeUnit terminator = u'\0';
    static constexpr auto codeUnitSize = sizeof(CodeUnit);
};

}
