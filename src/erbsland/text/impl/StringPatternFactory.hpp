// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StaticStringPatternData.hpp"
#include "StringPatternData.hpp"

#include "../pattern/AnyElement.hpp"
#include "../pattern/Limits.hpp"
#include "../u16/U16StringView_fwd.hpp"
#include "../u32/U32StringView_fwd.hpp"
#include "../u8/U8StringView_fwd.hpp"

#include <memory>

namespace erbsland::text::impl {

[[nodiscard]] auto createStringPatternData(const U8StringView &pattern) -> StringPatternDataPtr;
[[nodiscard]] auto createStringPatternData(const U16StringView &pattern) -> StringPatternDataPtr;
[[nodiscard]] auto createStringPatternData(const U32StringView &pattern) -> StringPatternDataPtr;

template <pattern::AnyElement... Args>
[[nodiscard]] auto createStaticStringPatternData(const Args &...elements) -> StringPatternDataPtr {
    using Data = StaticStringPatternData<pattern::cMaximumStaticElements, pattern::cMaximumStaticRanges>;
    return std::make_shared<Data>(elements...);
}

}
