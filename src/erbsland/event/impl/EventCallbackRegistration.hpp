// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventSubscriptionControl.hpp"

#include <atomic>
#include <memory>
#include <utility>

namespace erbsland::event::impl {

/// Shared publisher lifetime marker for callback registrations.
/// @tested{EventSubscriptionTest}
class EventCallbackListLifetime final {};

/// One callback registration owned by an event subscription.
/// @tparam tCallback The stored callback type.
/// @tested{EventSubscriptionTest}
template <typename tCallback>
class EventCallbackRegistration final : public EventSubscriptionControl {
public:
    /// Create an active registration for a publisher lifetime.
    /// @param lifetime The publisher lifetime marker.
    /// @param callback The registered callback.
    EventCallbackRegistration(std::weak_ptr<EventCallbackListLifetime> lifetime, tCallback callback) :
        _lifetime{std::move(lifetime)}, _callback{std::move(callback)} {}

public: // implement EventSubscriptionControl
    /// Cancel this registration.
    void cancel() noexcept override { _active.store(false, std::memory_order_release); }
    /// Test whether this registration and its publisher are still active.
    [[nodiscard]] auto isActive() const noexcept -> bool override {
        return _active.load(std::memory_order_acquire) && !_lifetime.expired();
    }

public:
    /// Access the stored callback.
    [[nodiscard]] auto callback() const noexcept -> const tCallback & { return _callback; }

private:
    std::weak_ptr<EventCallbackListLifetime> _lifetime; ///< Publisher lifetime marker.
    tCallback _callback;                                ///< Registered callback.
    std::atomic<bool> _active{true};                    ///< Whether dispatch is still allowed.
};

}
