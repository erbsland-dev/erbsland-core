// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::conf::syntax {

/// Formatting choice for WordCase.
enum class WordCase : uint8_t {
    Keep,  ///< Preserve spelling.
    Lower, ///< Lowercase words.
    Title, ///< Capitalize each word.
    Upper, ///< Uppercase words.
};

}
