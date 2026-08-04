// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <string_view>

namespace erbsland::unit::impl {

/// Throw an overflow error with the supplied reason.
[[noreturn]] void throwOverflow(std::string_view reason);

}
