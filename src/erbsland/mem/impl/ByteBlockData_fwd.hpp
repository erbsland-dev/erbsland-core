// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SharedByteDataWithFlag_fwd.hpp"

#include "../Byte.hpp"
#include "../SharedDataPointer.hpp"

namespace erbsland::mem::impl {

/// Shared byte data for `ByteBlockEditor` and `ByteBlock`.
using ByteBlockData = SharedByteDataWithFlag<Byte>;

/// Shared pointer to byte block data.
using ByteBlockDataPtr = SharedDataPointer<ByteBlockData>;

}
