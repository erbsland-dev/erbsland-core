// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::text {

/// Select how a string splitter handles separator characters.
enum class StringSplitMode : uint8_t {
    DiscardSeparator, ///< Exclude the separator from each returned part.
    KeepSeparator,    ///< Include the separator at the end of each returned part.
};

}
