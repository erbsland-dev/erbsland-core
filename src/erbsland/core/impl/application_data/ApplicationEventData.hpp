// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationEventData_fwd.hpp"

#include "../../../event/EventRegistry.hpp"
#include "../../../event/EventThread_fwd.hpp"
#include "../../../event/impl/EventLoop.hpp"
#include "../../../unit/ExitCode.hpp"

#include <exception>
#include <mutex>
#include <vector>

namespace erbsland::core::impl {

/// Main event loop, registry, managed threads, and application exit state.
/// @tested{ApplicationEventTest ApplicationPartApplicationTest}
class ApplicationEventData final {
public:
    /// Create the main event loop and its private event registry.
    ApplicationEventData();

public:
    /// Access the event target of the main event loop.
    [[nodiscard]] auto events() const noexcept -> const event::EventLoopPtr &;
    /// Access the main event loop.
    [[nodiscard]] auto eventLoop() noexcept -> event::EventLoop &;
    /// Access the private application event registry.
    [[nodiscard]] auto eventRegistry() noexcept -> event::EventRegistry &;
    /// Create and retain a managed event thread.
    [[nodiscard]] auto createEventThread() -> event::ManagedEventThreadPtr;
    /// Run the main event loop and capture its first callback failure.
    void runLoop();
    /// Test whether the main event loop has a pending quit request.
    [[nodiscard]] auto isQuitRequested() const noexcept -> bool;
    /// Finish event processing, join managed threads, and rethrow a captured loop error.
    [[nodiscard]] auto finishRun() -> unit::ExitCode;
    /// Preserve the first application exit code.
    void recordExitCode(unit::ExitCode exitCode) noexcept;
    /// Request shutdown of the main loop and every managed event thread.
    void quit() noexcept;

private:
    /// Collect live managed event threads and remove expired weak references.
    [[nodiscard]] auto managedThreads() -> std::vector<event::ManagedEventThreadPtr>;

private:
    event::EventLoopPtr _eventLoop;                         ///< Main application event loop.
    event::EventRegistry _eventRegistry;                    ///< Private application event registry.
    mutable std::mutex _mutex;                              ///< Protects managed state.
    bool _quitExitCodeSet{false};                           ///< Whether `quit()` supplied an exit code.
    unit::ExitCode _quitExitCode;                           ///< First exit code supplied to `quit()`.
    std::exception_ptr _loopError;                          ///< First callback failure from the main loop.
    std::vector<event::ManagedEventThreadWeakPtr> _threads; ///< Managed event threads.
};

}
