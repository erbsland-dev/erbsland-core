// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Byte.hpp"
#include "../SharedArrayData_fwd.hpp"
#include "../SharedDataPointer.hpp"

#include <cstdint>

namespace erbsland::mem::impl {

/// Shared byte data for `ByteBlockEditor` and `ByteBlock`.
using ByteBlockData = SharedArrayData<Byte, uint64_t>;

/// Shared pointer to byte block data.
using ByteBlockDataPtr = SharedDataPointer<ByteBlockData>;

}
