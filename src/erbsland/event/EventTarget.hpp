// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Event.hpp"
#include "EventBackend.hpp"
#include "EventBackendId.hpp"
#include "EventCallback.hpp"
#include "EventScheduler.hpp"
#include "EventTarget_fwd.hpp"
#include "EventTimer_fwd.hpp"

#include "../err/ParameterFailure.hpp"
#include "../text/Literals.hpp"
#include "../time/TimeDelta.hpp"

#include <utility>

namespace erbsland::event {

/// A target for thread-safe event posting and callback invocation.
/// @tested{ApplicationEventTest EventLoopTest EventThreadTest EventTimerTest}
class EventTarget {
public:
    // defaults
    virtual ~EventTarget() = default;

public: // interface
    /// Post an event to this target.
    /// @param event The event to post.
    virtual void post(Event event) = 0;
    /// Invoke a callback on this target.
    /// @param callback The callback to execute on the target event loop.
    virtual void invoke(EventCallback callback) = 0;
    /// Invoke a callback on this target after a delay.
    /// A zero or negative delay queues the callback like `invoke()`.
    /// @param delay The delay before queuing the callback.
    /// @param callback The callback to execute on the target event loop.
    virtual void invokeAfter(time::TimeDelta delay, EventCallback callback) = 0;
    /// Access a backend frontend interface.
    /// The requested backend must be available and expose the interface `T`.
    /// @tparam T The public backend frontend interface.
    /// @return The requested backend frontend.
    template <typename T>
    [[nodiscard]] auto get() -> T & {
        using namespace text::literals;
        auto &backend = getBackend(T::backendId());
        const auto result = dynamic_cast<T *>(&backend);
        if (result == nullptr) {
            throw err::ParameterFailure{"The backend does not implement the requested interface."_el, "backend"_el};
        }
        return *result;
    }
    /// Create an inactive timer for this target.
    /// @param callback The callback to execute when the timer fires.
    [[nodiscard]] auto createTimer(EventCallback callback) -> EventTimerPtr {
        return get<EventScheduler>().createTimer(std::move(callback));
    }

private:
    /// Get the backend implementation with the given identifier.
    [[nodiscard]] virtual auto getBackend(EventBackendId backendId) -> EventBackend & = 0;
};

}
