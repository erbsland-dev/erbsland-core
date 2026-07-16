// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8StringData_fwd.hpp"

#include "../../../unit/ByteIndex.hpp"
#include "../../../unit/ByteLength.hpp"
#include "../../../unit/ByteRange.hpp"
#include "../../../unit/CpIndex.hpp"
#include "../../../unit/CpLength.hpp"
#include "../../../unit/CpRange.hpp"

#include <cstddef>
#include <string_view>

namespace erbsland::text::impl {

/// Type bindings for UTF-8 string implementations.
/// @tested{U8StringDataTest}
struct U8StringTraits final {
    using CodeUnit = char;
    using StorageData = U8StringData;
    using StoragePtr = U8StringDataPtr;
    using StandardView = std::string_view;
    using StandardU8View = std::u8string_view;
    using DataIndex = unit::ByteIndex;
    using DataLength = unit::ByteLength;
    using DataRange = unit::ByteRange;
    using CharacterIndex = unit::CpIndex;
    using CharacterLength = unit::CpLength;
    using CharacterRange = unit::CpRange;

    static constexpr CodeUnit terminator = '\0';
    static constexpr auto codeUnitSize = sizeof(CodeUnit);
};

}
