// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::event::impl {

/// Internal control interface for an event subscription.
/// @tested{EventSubscriptionTest}
class EventSubscriptionControl {
public:
    // defaults
    virtual ~EventSubscriptionControl() = default;

public:
    /// Cancel this subscription.
    virtual void cancel() noexcept = 0;
    /// Test whether this subscription is still active.
    [[nodiscard]] virtual auto isActive() const noexcept -> bool = 0;
};

using EventSubscriptionControlPtr = std::shared_ptr<EventSubscriptionControl>;

}
