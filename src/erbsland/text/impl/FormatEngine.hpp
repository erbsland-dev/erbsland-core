// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FormatData.hpp"
#include "FormatMakeArguments.hpp"

#include "../StringBuilder.hpp"
#include "../u16/U16StringView.hpp"
#include "../u32/U32StringView.hpp"
#include "../u8/U8StringView.hpp"

#include <span>

namespace erbsland::text::impl {

/// Compile a format pattern.
[[nodiscard]] auto compileFormat(const U8StringView &pattern) -> FormatDataPtr;
/// Compile a format pattern.
[[nodiscard]] auto compileFormat(const U16StringView &pattern) -> FormatDataPtr;
/// Compile a format pattern.
[[nodiscard]] auto compileFormat(const U32StringView &pattern) -> FormatDataPtr;
/// Append a compiled format with the given arguments.
auto appendFormat(const FormatData &format, StringBuilder &builder, std::span<const FormatArgument> arguments)
    -> StringBuilder &;

}
