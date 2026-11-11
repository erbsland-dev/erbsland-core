// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DigitHistory.hpp"
#include "PiWorker.hpp"

#include <erbsland/CpCount.hpp>
#include <erbsland/EventTarget.hpp>
#include <erbsland/String.hpp>

#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <optional>

namespace demo {

/// Own required calculation results on main while publishing optional display notifications.
/// The application retains this session through worker join, including exception cleanup.
/// @tested{PiSessionTest}
class PiSession final : public std::enable_shared_from_this<PiSession> {
public:
    /// Notify an optional display that the shared main-loop history or calculation state changed.
    using OnUpdated = std::function<void()>;
    /// Notify the application after the worker has finished and joined.
    using OnFinished = std::function<void(std::exception_ptr)>;

public:
    /// Create a main-loop session with independently retained history.
    /// @param events The receiving loop, retained throughout the operation.
    /// @param history Main-loop digit storage.
    /// @param onUpdated Optional notification, normally bound weakly to a view.
    /// @param onFinished Required owner notification; the application survives the session.
    /// @param advance Optional alternate arithmetic for tests.
    PiSession(
        el::EventTargetPtr events,
        std::shared_ptr<DigitHistory> history,
        OnUpdated onUpdated,
        OnFinished onFinished,
        PiWorker::Advance advance = {});

public: // accessors
    /// Test whether final completion has been processed on main.
    [[nodiscard]] auto isFinished() const noexcept -> bool { return _finished; }
    /// Test whether cancellation has been requested on main.
    [[nodiscard]] auto isStopping() const noexcept -> bool { return _stopping; }

public: // lifecycle
    /// Start calculation on main with strongly bound required callbacks.
    /// @param limit Optional finite number of fractional digits.
    void start(std::optional<el::CpCount> limit);
    /// Request cooperative cancellation without closing main's queue; safe to repeat on main.
    void requestStop();
    /// Cancel and join without requiring another callback; call before releasing the session on cleanup.
    void shutdown();

private:
    /// Accept a required batch on main before requesting the next one.
    void onDigits(el::String digits);
    /// Join and retire operation state before notifying the application.
    void onFinished(std::exception_ptr error);

private:
    el::EventTargetPtr _events;             ///< Main-loop receiving target.
    std::shared_ptr<DigitHistory> _history; ///< History owned on main, independent of display lifetime.
    OnUpdated _onUpdated;                   ///< Optional weakly bound display notification.
    OnFinished _onFinished;                 ///< Required application continuation, invoked on main.
    PiWorker _worker;                       ///< Worker retained until main joins it.
    bool _started{false};                   ///< One-shot owner-loop state.
    bool _stopping{false};                  ///< Stop requested, completion may still be pending.
    bool _finished{false};                  ///< Required completion processed and worker joined.
};

}
