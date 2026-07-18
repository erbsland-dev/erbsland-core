// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringPatternFactory.hpp"

#include "DynamicStringPatternData.hpp"

#include "../StringCharReader.hpp"
#include "../u16/U16String.hpp"
#include "../u32/U32String.hpp"
#include "../u8/U8String.hpp"

namespace erbsland::text::impl {

auto createStringPatternData(const U8String &pattern) -> StringPatternDataPtr {
    auto reader = StringCharReader{pattern};
    return DynamicStringPatternData::parse(reader);
}

auto createStringPatternData(const U16String &pattern) -> StringPatternDataPtr {
    auto reader = StringCharReader{pattern};
    return DynamicStringPatternData::parse(reader);
}

auto createStringPatternData(const U32String &pattern) -> StringPatternDataPtr {
    auto reader = StringCharReader{pattern};
    return DynamicStringPatternData::parse(reader);
}

}
