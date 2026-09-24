// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ByteBlockData_fwd.hpp"

#include "../ByteBlockLiteral.hpp"

#include <variant>

namespace erbsland::mem::impl {

/// Shared heap or static literal storage for a read-only byte block.
using ByteBlockStorage = std::variant<std::monostate, ByteBlockDataPtr, ByteBlockLiteral>;

}
