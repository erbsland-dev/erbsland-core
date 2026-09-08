// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProcessSignal.hpp"
#include "ProcessSignalBackend_fwd.hpp"

#include "../../../event/EventSubscription.hpp"
#include "../../../event/impl/EventCallbackList.hpp"

#include <functional>
#include <memory>

namespace erbsland::system::impl {

/// Process-wide multiplexer for native termination inputs.
/// Native handlers forward to a worker thread before subscribers are invoked.
/// @tested{PosixApplicationServiceLifecycleTest WindowsApplicationServiceLifecycleTest}
class ProcessSignalDispatcher final {
public:
    using SignalFn = std::function<void(ProcessSignal signal, bool &claimed)>;

public:
    /// Access the process-wide signal dispatcher.
    [[nodiscard]] static auto instance() -> ProcessSignalDispatcher &;

    /// Stop the native process-signal backend.
    ~ProcessSignalDispatcher();

    // defaults/deletions
    ProcessSignalDispatcher(const ProcessSignalDispatcher &) = delete;
    ProcessSignalDispatcher(ProcessSignalDispatcher &&) = delete;
    auto operator=(const ProcessSignalDispatcher &) -> ProcessSignalDispatcher & = delete;
    auto operator=(ProcessSignalDispatcher &&) -> ProcessSignalDispatcher & = delete;

public:
    /// Add one process-signal observer.
    /// @param signalFn Callback invoked on the backend worker thread.
    /// @return A subscription retaining the observer.
    [[nodiscard]] auto addSignal(SignalFn signalFn) -> event::EventSubscription;

private:
    /// Create the process-wide dispatcher and native backend.
    ProcessSignalDispatcher();
    /// Dispatch one normalized signal and apply native default handling when unclaimed.
    void handleSignal(ProcessSignal signal) noexcept;

private:
    event::impl::EventCallbackList<SignalFn> _callbacks; ///< Process signal observers.
    std::unique_ptr<ProcessSignalBackend> _backend;      ///< Active native forwarding backend.
};

}
