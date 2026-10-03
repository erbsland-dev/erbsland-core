// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/String.hpp>
#include <erbsland/StringList.hpp>

namespace tool::doc::impl::test_status {

/// Normalize coverage marker syntax and remove markers attached to members.
/// @param text Original header text.
/// @return Normalized header text.
auto normalize(const el::String &text) -> el::String;
/// Validate marker values, placement, and line structure.
/// @param text Header text.
/// @return Diagnostics with one-based source lines.
auto validate(const el::String &text) -> el::StringList;

}
