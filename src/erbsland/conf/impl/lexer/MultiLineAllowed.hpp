// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::conf::impl::lexer {

/// If multi-line values are allowed.
enum class MultiLineAllowed : uint8_t { Yes, No };

}
