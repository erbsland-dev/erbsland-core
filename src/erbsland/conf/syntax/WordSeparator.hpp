// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::conf::syntax {

/// Formatting choice for WordSeparator.
enum class WordSeparator : uint8_t {
    Space,      ///< Separate words with spaces.
    Underscore, ///< Separate words with underscores.
};

}
