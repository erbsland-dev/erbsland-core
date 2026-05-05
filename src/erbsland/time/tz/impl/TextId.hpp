// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::time::tz::impl {

/// Identifier for an entry in the generated text table.
/// @notest{Internal generated-data helper.}
using TextId = uint16_t;

inline constexpr auto cEmptyTextId = TextId{0};

}
