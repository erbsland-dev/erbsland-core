// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TerminalStreamData_fwd.hpp"

#include "../BlockStyle.hpp"
#include "../Terminal_fwd.hpp"
#include "../TerminalStreamSynchronization.hpp"

#include "../../stream/OutputStreamSettings.hpp"
#include "../../stream/StreamState.hpp"
#include "../../text/String.hpp"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>

namespace erbsland::cterm::impl {

/// Shared state and background operations for a terminal text stream.
/// @tested{TerminalStreamTest}
class TerminalStreamData final : public std::enable_shared_from_this<TerminalStreamData> {
public:
    /// One queued terminal write command.
    struct Command final {
        text::String text;     ///< The text to write.
        BlockStyle style;      ///< The style applied to the text.
        bool lineBreak{false}; ///< Whether to append a terminal line break.
    };

public:
    /// Create the shared terminal stream state.
    TerminalStreamData(
        TerminalPtr terminal,
        BlockStyle style,
        TerminalStreamSynchronizationPtr synchronization,
        stream::OutputStreamSettings settings);

public:
    /// Schedule the next queued write or a pending close operation.
    void schedule();
    /// Execute one queued terminal write on an I/O worker.
    void performWrite(const Command &command);
    /// Schedule a terminal flush on an I/O worker.
    void scheduleFlush(bool closeAfterFlush, uint64_t generation);
    /// Execute one terminal flush on an I/O worker.
    void performFlush(bool closeAfterFlush, uint64_t generation);
    /// Get the output length represented by a command.
    [[nodiscard]] static auto commandLength(const Command &command) noexcept -> std::size_t;

public:
    TerminalPtr terminal;                             ///< The terminal receiving output.
    BlockStyle style;                                 ///< The style applied to newly queued output.
    TerminalStreamSynchronizationPtr synchronization; ///< The shared terminal synchronization state.
    stream::OutputStreamSettings settings;            ///< The immutable stream settings.
    mutable std::mutex mutex;                         ///< Protects the queue and non-atomic stream state.
    std::condition_variable condition;                ///< Signals capacity, completion, failure, and state changes.
    std::deque<Command> queue;                        ///< The queued terminal write commands.
    std::size_t pendingBytes{0U};                     ///< The number of queued bytes.
    std::atomic<stream::StreamState> streamState{stream::StreamState::Open}; ///< The stream lifecycle state.
    std::atomic<bool> aborted{false};      ///< Whether pending work must be abandoned.
    bool workInProgress{false};            ///< Whether an I/O worker owns a scheduled operation.
    uint64_t flushGeneration{0U};          ///< The most recently requested flush generation.
    uint64_t completedFlushGeneration{0U}; ///< The most recently completed flush generation.
    std::exception_ptr error;              ///< The failure captured by a background operation.
};

}
