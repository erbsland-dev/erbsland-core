// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../text/String.hpp"

namespace erbsland::cterm::impl {

/// Read one line from the redirected standard input stream.
/// @return The decoded line without one trailing terminal line ending, or an empty string at end of input.
/// @tested{BackendTest}
[[nodiscard]] auto readStandardInputLine() -> text::String;

}
