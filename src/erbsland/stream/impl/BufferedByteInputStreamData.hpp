// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BufferedByteInputStreamData_fwd.hpp"
#include "NativeByteStream.hpp"

#include "../InputStreamSettings.hpp"
#include "../StreamState.hpp"

#include "../../mem/RingBuffer.hpp"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>

namespace erbsland::stream::impl {

/// Shared state and background operations for a buffered byte input stream.
/// @tested{BufferedStreamTest}
class BufferedByteInputStreamData final : public std::enable_shared_from_this<BufferedByteInputStreamData> {
public:
    /// Create the shared input state.
    /// @param nativeStream The synchronous native stream used for source reads.
    /// @param streamSettings The immutable settings for the buffered stream.
    BufferedByteInputStreamData(NativeByteStreamPtr nativeStream, InputStreamSettings streamSettings);

public:
    /// Schedule a source read when the back ring can accept one.
    void scheduleRead();
    /// Perform one source read on an I/O worker.
    void performRead();

public:
    NativeByteStreamPtr native;                              ///< Synchronous native source.
    InputStreamSettings settings;                            ///< Immutable stream settings.
    mutable std::mutex mutex;                                ///< Protects rings and non-atomic state.
    std::condition_variable condition;                       ///< Signals data, completion, failure, or state changes.
    mem::RingBuffer front;                                   ///< Data available to public read operations.
    mem::RingBuffer back;                                    ///< Storage filled by the current background read.
    std::atomic<StreamState> streamState{StreamState::Open}; ///< Current stream lifecycle state.
    std::atomic<bool> aborted{false};                        ///< Whether pending work must be abandoned.
    std::atomic<uint64_t> logicalPosition{0U};               ///< Caller-independent logical byte position.
    bool readInProgress{false}; ///< Whether a background read currently owns the back ring.
    bool positioning{false};    ///< Whether a positioning operation prevents new read-ahead.
    bool finished{false};       ///< Whether the native source reached end-of-stream.
    std::exception_ptr error;   ///< Failure captured by the background read.
};

}
