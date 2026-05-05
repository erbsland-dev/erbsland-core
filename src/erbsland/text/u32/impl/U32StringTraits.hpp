// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../mem/SharedArrayData.hpp"
#include "../../../mem/SharedDataPointer.hpp"
#include "../../../unit/CpIndex.hpp"
#include "../../../unit/CpLength.hpp"
#include "../../../unit/CpRange.hpp"

#include <cstddef>
#include <string_view>

namespace erbsland::text::impl {

/// Type bindings for UTF-32 string implementations.
/// @tested{U32StringDataTest}
struct U32StringTraits final {
    using CodeUnit = char32_t;
    using StorageData = mem::SharedArrayData<CodeUnit>;
    using StoragePtr = mem::SharedDataPointer<StorageData>;
    using StandardView = std::u32string_view;
    using LiteralChar = char32_t;
    using DataIndex = unit::CpIndex;
    using DataLength = unit::CpLength;
    using DataRange = unit::CpRange;
    using CharacterIndex = unit::CpIndex;
    using CharacterLength = unit::CpLength;
    using CharacterRange = unit::CpRange;

    static constexpr CodeUnit terminator = U'\0';
    static constexpr auto codeUnitSize = sizeof(CodeUnit);
};

}
