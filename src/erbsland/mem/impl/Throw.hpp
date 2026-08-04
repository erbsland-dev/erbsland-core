// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <string_view>

namespace erbsland::mem::impl {

/// Throw an out of range error (slow, reason copy)
/// Only use this call in templates to prevent a circular include dependency.
[[noreturn]] void throwOutOfRange(std::string_view reason);

/// Throw an overflow error (slow, reson copy).
/// Only use from template methods to avoid circular include dependency.
[[noreturn]] void throwOverflow(std::string_view reason);

}
