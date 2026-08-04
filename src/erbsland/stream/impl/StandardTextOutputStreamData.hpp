// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "NativeOutputStream.hpp"
#include "StandardTextOutputStreamData_fwd.hpp"

#include "../OutputStreamSettings.hpp"
#include "../StreamState.hpp"

#include "../../text/String.hpp"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>

namespace erbsland::stream::impl {

/// Shared state and background operations for a standard text output stream.
/// @tested{StandardTextOutputStreamTest StandardStreamsTest}
class StandardTextOutputStreamData final : public std::enable_shared_from_this<StandardTextOutputStreamData> {
public:
    /// Create shared standard text output state.
    explicit StandardTextOutputStreamData(NativeOutputStreamPtr output);

public:
    /// Schedule the next queued write or a pending close operation.
    void schedule();
    /// Execute one queued write on an I/O worker.
    void performWrite(const text::String &text);
    /// Schedule a native flush on an I/O worker.
    void scheduleFlush(bool closeAfterFlush, uint64_t generation);
    /// Execute one native flush on an I/O worker.
    void performFlush(bool closeAfterFlush, uint64_t generation);

public:
    NativeOutputStreamPtr native;      ///< The synchronous native output target.
    OutputStreamSettings settings;     ///< The immutable stream settings.
    mutable std::mutex mutex;          ///< Protects the queue and non-atomic stream state.
    std::condition_variable condition; ///< Signals capacity, completion, failure, and state changes.
    std::deque<text::String> queue;    ///< The queued text writes.
    std::size_t pendingBytes{0U};      ///< The number of queued bytes.
    std::atomic<StreamState> streamState{StreamState::Open}; ///< The stream lifecycle state.
    std::atomic<bool> aborted{false};                        ///< Whether pending work must be abandoned.
    bool workInProgress{false};                              ///< Whether an I/O worker owns a scheduled operation.
    uint64_t flushGeneration{0U};                            ///< The most recently requested flush generation.
    uint64_t completedFlushGeneration{0U};                   ///< The most recently completed flush generation.
    std::exception_ptr error;                                ///< The failure captured by a background operation.
};

}
