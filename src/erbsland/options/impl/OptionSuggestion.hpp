// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../text/StringList.hpp"

namespace erbsland::options::impl {

/// Find up to three case-insensitive suggestions using the standard name-length threshold.
/// @tested{OptionsParserTest}
[[nodiscard]] auto findOptionSuggestions(const text::String &pattern, const text::StringList &candidates)
    -> text::StringList;

}
