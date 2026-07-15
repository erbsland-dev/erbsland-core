// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <DemoCommon.hpp>
#include <erbsland/debug/StringDebug.hpp>

namespace demo {

/// Print the most basic debug information about a string view.
inline void printMemoryAndRangeInfo(const el::StringView &text) {
    constexpr auto cDebugDetails =
        el::DebugViewDetail::BackingStore | el::DebugViewDetail::UnderlyingType | el::DebugViewDetail::Range;
    constexpr auto cDebugIndentWidth = el::CpLength{4U};
    constexpr auto cDebugInitialIndentWidth = el::CpLength{8U};
    el::io::printLine(el::toDebugString(text, cDebugDetails, cDebugIndentWidth, cDebugInitialIndentWidth));
}

}
