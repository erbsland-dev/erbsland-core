// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "InputStream_fwd.hpp"
#include "InputStreamSettings.hpp"
#include "StreamCloseStatus.hpp"
#include "StreamError.hpp"
#include "StreamPositioning.hpp"
#include "StreamState.hpp"
#include "StreamWaitStatus.hpp"

#include "../err/LogicError.hpp"

namespace erbsland::stream {

/// The common base class for readable streams.
/// All public operations are bounded by the timeout fixed in the stream settings. Destruction never waits for pending
/// native work; every concrete implementation must abort its backing operation from its destructor.
/// @tested{AsyncStreamTest}
class InputStream : public StreamPositioning, public std::enable_shared_from_this<InputStream> {
public:
    // defaults
    ~InputStream() override = default;

public: // state
    /// Get the immutable settings selected when this stream was created.
    [[nodiscard]] virtual auto inputSettings() const noexcept -> const InputStreamSettings & = 0;
    /// Get the lifecycle state.
    [[nodiscard]] virtual auto state() const noexcept -> StreamState = 0;
    /// Test if the stream is open for reading.
    [[nodiscard]] auto isOpen() const noexcept -> bool { return state() == StreamState::Open; }
    /// Test if a read can immediately return data or a final state.
    [[nodiscard]] virtual auto isReady() const noexcept -> bool = 0;
    /// Wait up to the configured timeout for a read to become ready.
    [[nodiscard]] virtual auto waitForReady() -> StreamWaitStatus = 0;

public: // lifecycle
    /// Start or continue graceful close and wait up to the configured timeout.
    virtual auto close() -> StreamCloseStatus = 0;
    /// Immediately abandon pending work without waiting.
    virtual void abort() noexcept = 0;

protected:
    /// Obtain shared ownership for a suspended coroutine operation.
    /// @return Shared ownership of this stream.
    /// @throws err::LogicError If this stream is not owned by a shared pointer.
    [[nodiscard]] auto sharedInputStream() -> InputStreamPtr {
        auto result = weak_from_this().lock();
        if (!result) {
            throw err::LogicError{"Coroutine input operations require a shared-owned stream."};
        }
        return result;
    }
};

}
