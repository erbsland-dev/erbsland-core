// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../unit/ByteLength.hpp"

#include <cstddef>
#include <cstdint>

namespace erbsland::log {

/// A point-in-time snapshot of manager counters.
/// @tested{LogCoreTest}
struct LogManagerStatistics final {
    uint64_t acceptedEntries{};     ///< Number of entries accepted into the queue.
    uint64_t writtenEntries{};      ///< Number of entries delivered to at least one writer.
    uint64_t droppedEntries{};      ///< Number of entries discarded due to limits or shutdown.
    uint64_t writerFailures{};      ///< Number of contained writer failures.
    std::size_t queuedEntries{};    ///< Current number of queued entries.
    unit::ByteLength queuedBytes{}; ///< Current message bytes retained in the queue.
};

}
