// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <string_view>

namespace erbsland::math::impl {

/// Throw an overflow error (slow, reson copy).
/// Only use from template methods to avoid circular include dependency.
[[noreturn]] void throwOverflow(std::string_view reason);

}
