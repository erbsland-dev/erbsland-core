// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::conf::impl::lexer {

/// What kind of characters are accepted ending a name.
enum class AcceptedNameEnd : uint8_t {
    NamePath, ///< Accept `.`, `[`, and end-of-data.
    Section,  ///< Accept `.` and `]` and end marks.
};

}
