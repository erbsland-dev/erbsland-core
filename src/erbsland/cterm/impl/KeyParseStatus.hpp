// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cterm::impl {

/// The status for a decoded terminal key prefix.
enum class KeyParseStatus : uint8_t {
    Invalid,      ///< No valid key prefix was found.
    NeedMoreData, ///< The current bytes are a valid prefix, but need more input.
    Parsed,       ///< A full key was parsed.
};

}
