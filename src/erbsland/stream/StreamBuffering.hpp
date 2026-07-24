// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

namespace erbsland::stream {

/// Selects the intended balance between stream memory use and throughput.
enum class StreamBuffering {
    MinimalMemory, ///< Minimize retained memory, accepting more frequent I/O.
    Interactive,   ///< Favor responsive streams with modest buffering.
    Balanced,      ///< Balance memory use and throughput for general use.
    Throughput,    ///< Favor throughput with larger buffers.
    Bulk,          ///< Maximize throughput for large sequential transfers.
};

}
