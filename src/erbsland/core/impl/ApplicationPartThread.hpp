// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../event/EventCallback.hpp"
#include "../../event/EventLoop_fwd.hpp"
#include "../../event/Events_fwd.hpp"

#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

namespace erbsland::core::impl {

/// A dedicated application-part event thread with a post-loop cleanup phase.
/// @tested{ApplicationPartManagerTest}
class ApplicationPartThread final {
public:
    /// A callback receiving event-loop failures.
    using ErrorFn = std::function<void(std::exception_ptr)>;

public:
    /// Create a part thread with lifecycle callbacks.
    ApplicationPartThread(event::EventCallback startup, event::EventCallback cleanup, ErrorFn errorFn);
    /// Stop and join this part thread.
    ~ApplicationPartThread() noexcept;

    // defaults/deletions
    ApplicationPartThread(const ApplicationPartThread &) = delete;
    auto operator=(const ApplicationPartThread &) -> ApplicationPartThread & = delete;
    ApplicationPartThread(ApplicationPartThread &&) = delete;
    auto operator=(ApplicationPartThread &&) -> ApplicationPartThread & = delete;

public:
    /// Start the dedicated thread.
    void start();
    /// Queue a stopping callback.
    void requestStop(event::EventCallback stopping);
    /// Request the event loop to quit.
    void quit() noexcept;
    /// Wait until the worker exits.
    void join();
    /// Access the part event target.
    [[nodiscard]] auto events() const noexcept -> event::EventsPtr;
    /// Get the worker thread identifier, if started.
    [[nodiscard]] auto threadId() const noexcept -> std::thread::id;

private:
    event::EventLoopPtr _eventLoop; ///< The part event loop.
    event::EventCallback _cleanup;  ///< Called after the event loop exits.
    ErrorFn _errorFn;               ///< Receives event-loop failures.
    mutable std::mutex _mutex;      ///< Protects thread ownership.
    std::thread _thread;            ///< The dedicated worker thread.
    bool _started{false};           ///< True after successful start.
};

using ApplicationPartThreadPtr = std::shared_ptr<ApplicationPartThread>;

}
