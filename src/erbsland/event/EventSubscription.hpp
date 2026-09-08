// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventSubscription_fwd.hpp"

#include "impl/EventSubscriptionControl.hpp"

#include <utility>

namespace erbsland::event {

/// A move-only lifetime handle for one registered event callback.
/// Destroying or cancelling this handle prevents future callback invocations. A callback that already started may
/// finish. Destroying the publisher automatically invalidates the subscription.
/// @tested{EventSubscriptionTest}
class EventSubscription final {
public:
    /// Create an inactive subscription.
    EventSubscription() = default;
    /// Cancel this subscription.
    ~EventSubscription() { cancel(); }

    // defaults/deletions
    EventSubscription(const EventSubscription &) = delete;
    auto operator=(const EventSubscription &) -> EventSubscription & = delete;
    EventSubscription(EventSubscription &&) noexcept = default;
    auto operator=(EventSubscription &&other) noexcept -> EventSubscription & {
        if (this != &other) {
            cancel();
            _control = std::move(other._control);
        }
        return *this;
    }

public:
    /// Cancel this subscription.
    void cancel() noexcept {
        if (_control != nullptr) {
            _control->cancel();
            _control.reset();
        }
    }
    /// Test whether this subscription is still active.
    [[nodiscard]] auto isActive() const noexcept -> bool { return _control != nullptr && _control->isActive(); }

public: // internal construction
    /// Create a subscription from its internal control.
    /// @param control The control owned by this subscription.
    explicit EventSubscription(impl::EventSubscriptionControlPtr control) noexcept : _control{std::move(control)} {}

private:
    impl::EventSubscriptionControlPtr _control; ///< Internal callback registration control.
};

}
