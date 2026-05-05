// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::text::impl {

/// The argument index mode used while parsing a UTF-8 format pattern.
/// @tested{U8FormatTest}
enum class IndexMode : uint8_t {
    None,
    Automatic,
    Manual,
};

}
