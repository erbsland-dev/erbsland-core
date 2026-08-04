// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OutputStream_fwd.hpp"
#include "OutputStreamSettings.hpp"
#include "StreamCloseStatus.hpp"
#include "StreamError.hpp"
#include "StreamPositioning.hpp"
#include "StreamState.hpp"
#include "StreamWaitStatus.hpp"
#include "StreamWriteStatus.hpp"

#include "../err/LogicError.hpp"

namespace erbsland::stream {

/// The common base class for writable streams.
/// Output streams atomically accept complete write requests. Public operations wait at most for the timeout fixed in
/// the settings. Destruction never waits for pending native work; every concrete implementation must abort from its
/// destructor. A successful write is queued for delivery; only a successful `flush()` confirms native flushing.
/// @tested{AsyncStreamTest}
class OutputStream : public StreamPositioning, public std::enable_shared_from_this<OutputStream> {
public:
    // defaults
    ~OutputStream() override = default;

public: // state
    /// Get the immutable settings selected when this stream was created.
    [[nodiscard]] virtual auto outputSettings() const noexcept -> const OutputStreamSettings & = 0;
    /// Get the lifecycle state.
    [[nodiscard]] virtual auto state() const noexcept -> StreamState = 0;
    /// Test if the stream is open for writing.
    [[nodiscard]] auto isOpen() const noexcept -> bool { return state() == StreamState::Open; }
    /// Test if the stream has no queued back-buffer data.
    /// For a single producer, a following write within `backBufferLimit()` can be accepted without waiting unless the
    /// stream state changes. Concurrent producers must inspect each write result.
    [[nodiscard]] virtual auto isReady() const noexcept -> bool = 0;
    /// Wait up to the configured timeout for the stream to become ready.
    [[nodiscard]] virtual auto waitForReady() -> StreamWaitStatus = 0;

public: // lifecycle
    /// Flush buffered output.
    /// @throws stream::StreamError If the backing target reports a flush error.
    virtual auto flush() -> StreamWriteStatus = 0;
    /// Start or continue graceful close and wait up to the configured timeout.
    virtual auto close() -> StreamCloseStatus = 0;
    /// Immediately abandon queued output and pending native work without waiting.
    virtual void abort() noexcept = 0;

protected:
    /// Obtain shared ownership for a suspended coroutine operation.
    /// @return Shared ownership of this stream.
    /// @throws err::LogicError If this stream is not owned by a shared pointer.
    [[nodiscard]] auto sharedOutputStream() -> OutputStreamPtr {
        auto result = weak_from_this().lock();
        if (!result) {
            throw err::LogicError{"Coroutine output operations require a shared-owned stream."};
        }
        return result;
    }
};

}
