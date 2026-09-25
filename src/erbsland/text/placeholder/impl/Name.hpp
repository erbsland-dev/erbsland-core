// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../String.hpp"

namespace erbsland::text::placeholder::impl {

/// Normalize a regular placeholder name or report a syntax error.
/// @tested{ReplacerTest ParserPlaceholderTest}
[[nodiscard]] auto normalizeName(const String &name) -> String;

}
