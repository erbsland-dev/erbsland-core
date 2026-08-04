// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DebugViewDetails.hpp"

#include "../text/StringTree.hpp"
#include "../text/u16/U16String.hpp"
#include "../text/u16/U16StringEditor.hpp"
#include "../text/u32/U32String.hpp"
#include "../text/u32/U32StringEditor.hpp"
#include "../text/u8/U8String.hpp"
#include "../text/u8/U8StringEditor.hpp"

namespace erbsland::debug {

/// Build a debug tree for a UTF-8 string editor.
/// @param value The value to inspect.
/// @param details The optional details to include.
/// @return The debug tree for `value`.
/// @tested{StringDebugTest}
[[nodiscard]] auto toDebugTree(const text::U8StringEditor &value, DebugViewDetails details = {}) -> text::StringTree;
/// @overload
[[nodiscard]] auto toDebugTree(const text::U8String &value, DebugViewDetails details = {}) -> text::StringTree;
/// @overload
[[nodiscard]] auto toDebugTree(const text::U16StringEditor &value, DebugViewDetails details = {}) -> text::StringTree;
/// @overload
[[nodiscard]] auto toDebugTree(const text::U16String &value, DebugViewDetails details = {}) -> text::StringTree;
/// @overload
[[nodiscard]] auto toDebugTree(const text::U32StringEditor &value, DebugViewDetails details = {}) -> text::StringTree;
/// @overload
[[nodiscard]] auto toDebugTree(const text::U32String &value, DebugViewDetails details = {}) -> text::StringTree;
/// Build a formatted debug string for a supported value.
/// @tparam T The type of value to inspect.
/// @param value The value to inspect.
/// @param details The optional details to include.
/// @param indentWidth The indentation width for each tree depth.
/// @param initialIndentWidth The indentation width before the root.
/// @return The formatted debug string for `value`.
/// @tested{StringDebugTest}
template <typename T>
[[nodiscard]] auto toDebugString(
    const T &value,
    DebugViewDetails details = DebugViewDetail::Default,
    unit::CpLength indentWidth = unit::CpLength{4U},
    unit::CpLength initialIndentWidth = unit::CpLength::zero()) -> text::String {
    return toDebugTree(value, details).toString(indentWidth, initialIndentWidth);
}

}
