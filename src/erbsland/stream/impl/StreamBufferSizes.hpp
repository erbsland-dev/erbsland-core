// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../StreamBuffering.hpp"

#include "../../unit/ByteCount.hpp"

namespace erbsland::stream::impl {

/// Internal buffer sizes resolved from a public buffering intention.
/// @tested{StreamSettingsTest}
struct StreamBufferSizes final {
    unit::ByteCount ioRing;                ///< Capacity for fixed input and output rings.
    unit::ByteCount aggregateChunk;        ///< Preferred chunk for aggregate byte reads.
    unit::ByteCount decoder;               ///< Capacity of encoded-text decode buffers.
    unit::ByteCount outputRetainedInitial; ///< Initial capacity of growing output storage.
    unit::ByteCount outputBackLimit;       ///< Default maximum retained output.
};

/// Resolve the internal sizes for a buffering intention.
/// @param buffering The public buffering intention.
/// @return The corresponding internal buffer sizes.
/// @tested{StreamSettingsTest}
[[nodiscard]] auto streamBufferSizes(StreamBuffering buffering) noexcept -> StreamBufferSizes;

}
