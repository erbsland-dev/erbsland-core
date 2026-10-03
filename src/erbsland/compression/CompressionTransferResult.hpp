// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../unit/ByteCount.hpp"

namespace erbsland::compression {

/// Final byte counts accepted by a compression transfer.
/// @tested{CompressionStreamingTest}
struct CompressionTransferResult final {
    unit::ByteCount inputLength;  ///< Bytes read from the source.
    unit::ByteCount outputLength; ///< Bytes accepted by the destination.
};

}
