// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FormatData.hpp"
#include "FormatMakeArguments.hpp"

#include "../AnyStringBuilder.hpp"
#include "../u16/U16String.hpp"
#include "../u32/U32String.hpp"
#include "../u8/U8String.hpp"

#include <span>

namespace erbsland::text::impl {

/// Compile a format pattern.
[[nodiscard]] auto compileFormat(const U8String &pattern) -> FormatDataPtr;
/// Compile a format pattern.
[[nodiscard]] auto compileFormat(const U16String &pattern) -> FormatDataPtr;
/// Compile a format pattern.
[[nodiscard]] auto compileFormat(const U32String &pattern) -> FormatDataPtr;
/// Append a compiled format with the given arguments.
auto appendFormat(const FormatData &format, AnyStringBuilder &builder, std::span<const FormatArgument> arguments)
    -> AnyStringBuilder &;

}
