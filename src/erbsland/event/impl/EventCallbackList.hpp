// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EventCallbackRegistration.hpp"

#include "../EventSubscription.hpp"

#include <algorithm>
#include <exception>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace erbsland::event::impl {

/// A thread-safe ordered collection of callback subscriptions.
/// The publisher supplies the callback-exception policy for each notification.
/// @tparam tCallback The callback type stored by this list.
/// @tested{EventSubscriptionTest}
template <typename tCallback>
class EventCallbackList final {
private:
    using Registration = EventCallbackRegistration<tCallback>;

public:
    /// Create an empty callback list.
    EventCallbackList() : _lifetime{std::make_shared<EventCallbackListLifetime>()} {}
    /// Cancel all callback registrations.
    ~EventCallbackList() { clear(); }

    // defaults/deletions
    EventCallbackList(const EventCallbackList &) = delete;
    EventCallbackList(EventCallbackList &&) = delete;
    auto operator=(const EventCallbackList &) -> EventCallbackList & = delete;
    auto operator=(EventCallbackList &&) -> EventCallbackList & = delete;

public:
    /// Add a callback subscription.
    /// @param callback The callback invoked by later notifications.
    /// @return A handle that retains the callback registration.
    [[nodiscard]] auto add(tCallback callback) -> EventSubscription {
        auto registration = std::make_shared<Registration>(_lifetime, std::move(callback));
        {
            const auto lock = std::scoped_lock{_mutex};
            removeInactiveLocked();
            _registrations.emplace_back(registration);
        }
        return EventSubscription{std::move(registration)};
    }

    /// Notify all active callbacks in registration order.
    /// Callback exceptions are forwarded individually and do not skip later callbacks.
    /// @tparam tErrorHandler A callable accepting `std::exception_ptr`.
    /// @tparam tArgs The callback argument types.
    /// @param errorHandler The publisher-defined callback-exception handler.
    /// @param args Arguments forwarded to every callback.
    template <typename tErrorHandler, typename... tArgs>
    void notify(tErrorHandler &&errorHandler, tArgs &&...args) {
        auto registrations = std::vector<std::shared_ptr<Registration>>{};
        {
            const auto lock = std::scoped_lock{_mutex};
            removeInactiveLocked();
            registrations.reserve(_registrations.size());
            for (const auto &registrationWeak : _registrations) {
                if (auto registration = registrationWeak.lock(); registration != nullptr) {
                    registrations.emplace_back(std::move(registration));
                }
            }
        }
        for (const auto &registration : registrations) {
            if (!registration->isActive()) {
                continue;
            }
            try {
                registration->callback()(args...);
            } catch (...) {
                std::forward<tErrorHandler>(errorHandler)(std::current_exception());
            }
        }
    }

    /// Cancel and remove every callback.
    void clear() noexcept {
        auto registrations = std::vector<std::shared_ptr<Registration>>{};
        {
            const auto lock = std::scoped_lock{_mutex};
            for (const auto &registrationWeak : _registrations) {
                if (auto registration = registrationWeak.lock(); registration != nullptr) {
                    registrations.emplace_back(std::move(registration));
                }
            }
            _registrations.clear();
        }
        for (const auto &registration : registrations) {
            registration->cancel();
        }
    }

private:
    /// Remove registrations that are cancelled or no longer retained.
    void removeInactiveLocked() {
        std::erase_if(_registrations, [](const std::weak_ptr<Registration> &registrationWeak) -> bool {
            const auto registration = registrationWeak.lock();
            return registration == nullptr || !registration->isActive();
        });
    }

private:
    mutable std::mutex _mutex;                               ///< Protects the ordered registration list.
    std::shared_ptr<EventCallbackListLifetime> _lifetime;    ///< Invalidates subscriptions with this publisher.
    std::vector<std::weak_ptr<Registration>> _registrations; ///< Registrations in notification order.
};

}
