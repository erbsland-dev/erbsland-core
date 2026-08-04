// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventData_fwd.hpp"

#include "../../event/EventRegistry.hpp"
#include "../../event/EventThread_fwd.hpp"
#include "../../event/impl/EventLoop.hpp"
#include "../../unit/ExitCode.hpp"

#include <memory>
#include <mutex>
#include <vector>

namespace erbsland::core::impl {

/// Data owned by the application's event subsystem.
/// @tested{ApplicationOptionsTest ApplicationTestScopeTest}
class EventData final {
public:
    /// Create the event loop and its private event registry.
    EventData() :
        eventLoop{std::make_shared<event::impl::EventLoop>()}, eventIdRegistry{event::EventRegistry::PrivateTag{}} {}

public:
    event::EventLoopPtr eventLoop;        ///< The application's event loop.
    event::EventRegistry eventIdRegistry; ///< The event registry.
    std::mutex mutex;                     ///< Protects managed event system state.
    bool quitExitCodeSet{false};          ///< True if the application exit code was set by `quit()`.
    unit::ExitCode quitExitCode;          ///< The first exit code passed to `quit()`.
    std::vector<event::ManagedEventThreadWeakPtr> eventThreads; ///< The managed event threads.
};

}
