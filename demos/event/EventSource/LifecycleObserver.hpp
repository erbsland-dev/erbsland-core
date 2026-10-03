// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/EventSubscription.hpp>

#include <utility>

namespace demo {

/// Own one lifecycle registration independently of the publisher.
/// @notest{Compiled documentation demo.}
class LifecycleObserver final {
public:
    /// Transfer a registration into this observer.
    /// @param subscription The registration to retain.
    explicit LifecycleObserver(el::EventSubscription subscription) : _subscription{std::move(subscription)} {}

public: // accessors
    /// Test whether the retained registration remains active.
    [[nodiscard]] auto isActive() const noexcept -> bool { return _subscription.isActive(); }

public: // operations
    /// Stop observing future notifications; safe to repeat.
    void stopWatching() noexcept { _subscription.cancel(); }

private:
    el::EventSubscription _subscription; ///< Cancels automatically when the observer is destroyed.
};

}
