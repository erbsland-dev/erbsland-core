// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BufferedByteOutputStreamData_fwd.hpp"
#include "NativeByteStream.hpp"

#include "../OutputStreamSettings.hpp"
#include "../StreamState.hpp"

#include "../../mem/RingBuffer.hpp"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>

namespace erbsland::stream::impl {

/// Shared state and background operations for a buffered byte output stream.
/// @tested{BufferedStreamTest}
class BufferedByteOutputStreamData final : public std::enable_shared_from_this<BufferedByteOutputStreamData> {
public:
    /// Create the shared output state.
    /// @param nativeStream The synchronous native stream used for target operations.
    /// @param streamSettings The immutable settings for the buffered stream.
    BufferedByteOutputStreamData(NativeByteStreamPtr nativeStream, OutputStreamSettings streamSettings);

public:
    /// Schedule buffered output or a pending close on an I/O worker.
    void scheduleWrite();
    /// Move queued back-ring bytes into the fixed front ring.
    void refillFront();
    /// Write one contiguous front-ring section on an I/O worker.
    void performWrite();
    /// Schedule a graceful native close after all buffered data was written.
    void scheduleClose();
    /// Flush and close the native stream on an I/O worker.
    void performClose();

public:
    NativeByteStreamPtr native;        ///< Synchronous native target.
    OutputStreamSettings settings;     ///< Immutable stream settings.
    mutable std::mutex mutex;          ///< Protects rings and non-atomic state.
    std::condition_variable condition; ///< Signals capacity, completion, failure, or state changes.
    mem::RingBuffer front;             ///< Fixed buffer consumed by the current background write.
    mem::RingBuffer back;              ///< Bounded growing queue for atomic byte and encoded-text writes.
    std::atomic<StreamState> streamState{StreamState::Open}; ///< Current stream lifecycle state.
    std::atomic<bool> aborted{false};                        ///< Whether pending work must be abandoned.
    std::atomic<uint64_t> logicalPosition{0U};               ///< Logical byte position including accepted output.
    bool workInProgress{false};            ///< Whether an I/O worker currently owns native or front data.
    bool positioning{false};               ///< Whether a positioning operation blocks new output.
    uint64_t flushGeneration{0U};          ///< Last flush request generation.
    uint64_t completedFlushGeneration{0U}; ///< Last successfully completed flush generation.
    std::exception_ptr error;              ///< Failure captured by background work.
};

}
