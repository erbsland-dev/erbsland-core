// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <DemoCommon.hpp>
#include <erbsland/EventTarget.hpp>

#include <functional>
#include <mutex>

namespace demo {

/// Coalesce producer updates into one latest value and at most one waiting notification.
/// Keep this object alive until producers have stopped and queued deliveries have finished.
/// The target must remain open with unlimited ordinary admission while publishing.
class LatestProgress final {
public:
    /// Retain an open target and its display callback.
    /// @param target The loop on which progress is displayed.
    /// @param display The callback invoked with the latest value, outside the state lock.
    LatestProgress(el::EventTargetPtr target, std::function<void(int)> display);

public: // operations
    /// Replace the latest value and schedule a notification only when none is pending.
    /// Safe to call from multiple producer threads; submission failures propagate to the caller.
    /// @param progress The newest progress percentage.
    void publish(int progress);

private:
    void deliver();

private:
    el::EventTargetPtr _target;        ///< Open target retained through all publications.
    std::function<void(int)> _display; ///< Display handler, invoked only on the receiving loop.
    std::mutex _mutex;                 ///< Protects the latest value and pending flag together.
    int _latest = 0;                   ///< Newest value supplied by a producer.
    bool _pending = false;             ///< Whether a notification is waiting to consume the value.
};

}
