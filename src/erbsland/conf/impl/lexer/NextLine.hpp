// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::conf::impl::lexer {

/// If the value is defined on the same or next line.
enum class NextLine : uint8_t { Yes, No };

}
