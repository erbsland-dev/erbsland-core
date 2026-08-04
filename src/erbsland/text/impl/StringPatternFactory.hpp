// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StaticStringPatternData.hpp"
#include "StringPatternData.hpp"

#include "../pattern/AnyElement.hpp"
#include "../pattern/Limits.hpp"
#include "../u16/U16String_fwd.hpp"
#include "../u32/U32String_fwd.hpp"
#include "../u8/U8String_fwd.hpp"

#include <memory>

namespace erbsland::text::impl {

/// Create runtime pattern data from UTF-8 text.
[[nodiscard]] auto createStringPatternData(const U8String &pattern) -> StringPatternDataPtr;
/// Create runtime pattern data from UTF-16 text.
[[nodiscard]] auto createStringPatternData(const U16String &pattern) -> StringPatternDataPtr;
/// Create runtime pattern data from UTF-32 text.
[[nodiscard]] auto createStringPatternData(const U32String &pattern) -> StringPatternDataPtr;

/// Create static pattern data from pattern elements.
template <pattern::AnyElement... Args>
[[nodiscard]] auto createStaticStringPatternData(const Args &...elements) -> StringPatternDataPtr {
    using Data = StaticStringPatternData<pattern::cMaximumStaticElements, pattern::cMaximumStaticRanges>;
    return std::make_shared<Data>(elements...);
}

}
