// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DebugViewDetails.hpp"

#include "../text/StringTree.hpp"
#include "../text/u16/U16String.hpp"
#include "../text/u16/U16StringView.hpp"
#include "../text/u32/U32String.hpp"
#include "../text/u32/U32StringView.hpp"
#include "../text/u8/U8String.hpp"
#include "../text/u8/U8StringView.hpp"

namespace erbsland::debug {

[[nodiscard]] auto toDebugTree(const text::U8String &value, DebugViewDetails details = {}) -> text::StringTree;
[[nodiscard]] auto toDebugTree(const text::U8StringView &value, DebugViewDetails details = {}) -> text::StringTree;
[[nodiscard]] auto toDebugTree(const text::U16String &value, DebugViewDetails details = {}) -> text::StringTree;
[[nodiscard]] auto toDebugTree(const text::U16StringView &value, DebugViewDetails details = {}) -> text::StringTree;
[[nodiscard]] auto toDebugTree(const text::U32String &value, DebugViewDetails details = {}) -> text::StringTree;
[[nodiscard]] auto toDebugTree(const text::U32StringView &value, DebugViewDetails details = {}) -> text::StringTree;

template <typename T>
[[nodiscard]] auto toDebugString(
    const T &value,
    DebugViewDetails details = DebugViewDetail::Default,
    unit::CpLength indentWidth = unit::CpLength{4U},
    unit::CpLength initialIndentWidth = unit::CpLength::zero()) -> text::String {
    return toDebugTree(value, details).toString(indentWidth, initialIndentWidth);
}

}
