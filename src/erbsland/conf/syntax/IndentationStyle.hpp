// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::conf::syntax {

/// Formatting choice for IndentationStyle.
enum class IndentationStyle : uint8_t {
    Spaces, ///< Indent with spaces.
    Tabs,   ///< Indent with tabs.
};

}
